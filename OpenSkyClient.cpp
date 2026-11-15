#include "OpenSkyClient.h"

#include <windows.h>
#include <winhttp.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <limits>
#include <regex>
#include <set>
#include <string_view>
#include <vector>

#include "UnitConverter.h"

#pragma comment(lib, "winhttp.lib")

namespace {
struct Credentials {
	std::string clientId;
	std::string clientSecret;
};

struct HttpResponse {
	DWORD status = 0;
	std::string body;
	std::string rateLimitRemaining;
	std::string retryAfterSeconds;
};

std::string readResponse(HINTERNET request) {
	std::string result;
	DWORD available = 0;
	while (WinHttpQueryDataAvailable(request, &available) && available != 0) {
		std::string buffer(available, '\0');
		DWORD read = 0;
		if (!WinHttpReadData(request, buffer.data(), available, &read)) return {};
		result.append(buffer.data(), read);
	}
	return result;
}

std::string jsonString(const std::string& json, const std::string& name) {
	const std::regex pattern("\\\"" + name + "\\\"\\s*:\\s*\\\"([^\\\"]+)\\\"");
	std::smatch match;
	return std::regex_search(json, match, pattern) ? match[1].str() : std::string{};
}

bool parseCredentials(const std::string& json, Credentials& credentials) {
	credentials.clientId = jsonString(json, "clientId");
	credentials.clientSecret = jsonString(json, "clientSecret");
	return !credentials.clientId.empty() && !credentials.clientSecret.empty();
}

std::string urlEncode(const std::string& value) {
	static constexpr char hex[] = "0123456789ABCDEF";
	std::string encoded;
	for (const unsigned char character : value) {
		if ((character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z')
			|| (character >= '0' && character <= '9') || character == '-' || character == '_' || character == '.' || character == '~') {
			encoded += static_cast<char>(character);
		}
		else {
			encoded += '%';
			encoded += hex[character >> 4];
			encoded += hex[character & 15];
		}
	}
	return encoded;
}

HttpResponse request(const wchar_t* host, const wchar_t* path, const std::string& body,
	const wchar_t* headers, bool post) {
	HINTERNET session = WinHttpOpen(L"MLSS-Aircraft-Replay/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (!session) return {};
	WinHttpSetTimeouts(session, 15000, 15000, 15000, 30000);
	HINTERNET connection = WinHttpConnect(session, host, INTERNET_DEFAULT_HTTPS_PORT, 0);
	HINTERNET handle = connection ? WinHttpOpenRequest(connection, post ? L"POST" : L"GET", path, nullptr,
		WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE) : nullptr;
	HttpResponse response;
	if (handle) {
		const DWORD bodySize = static_cast<DWORD>(body.size());
		if (WinHttpSendRequest(handle, headers, static_cast<DWORD>(-1L),
			post ? const_cast<char*>(body.data()) : nullptr, post ? bodySize : 0, post ? bodySize : 0, 0)
			&& WinHttpReceiveResponse(handle, nullptr)) {
			DWORD statusSize = sizeof(response.status);
			WinHttpQueryHeaders(handle, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
				WINHTTP_HEADER_NAME_BY_INDEX, &response.status, &statusSize, WINHTTP_NO_HEADER_INDEX);
			DWORD headerSize = 0;
			WinHttpQueryHeaders(handle, WINHTTP_QUERY_CUSTOM, L"X-Rate-Limit-Remaining",
				nullptr, &headerSize, WINHTTP_NO_HEADER_INDEX);
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
				std::wstring header(headerSize / sizeof(wchar_t), L'\0');
				if (WinHttpQueryHeaders(handle, WINHTTP_QUERY_CUSTOM, L"X-Rate-Limit-Remaining",
					header.data(), &headerSize, WINHTTP_NO_HEADER_INDEX)) {
					if (!header.empty() && header.back() == L'\0') header.pop_back();
					response.rateLimitRemaining.assign(header.begin(), header.end());
				}
			}
			headerSize = 0;
			WinHttpQueryHeaders(handle, WINHTTP_QUERY_CUSTOM, L"X-Rate-Limit-Retry-After-Seconds",
				nullptr, &headerSize, WINHTTP_NO_HEADER_INDEX);
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
				std::wstring header(headerSize / sizeof(wchar_t), L'\0');
				if (WinHttpQueryHeaders(handle, WINHTTP_QUERY_CUSTOM, L"X-Rate-Limit-Retry-After-Seconds",
					header.data(), &headerSize, WINHTTP_NO_HEADER_INDEX)) {
					if (!header.empty() && header.back() == L'\0') header.pop_back();
					response.retryAfterSeconds.assign(header.begin(), header.end());
				}
			}
			response.body = readResponse(handle);
		}
	}
	if (handle) WinHttpCloseHandle(handle);
	if (connection) WinHttpCloseHandle(connection);
	WinHttpCloseHandle(session);
	return response;
}

std::string bearerToken(const Credentials& credentials, std::string& error) {
	const std::string body = "grant_type=client_credentials&client_id=" + urlEncode(credentials.clientId)
		+ "&client_secret=" + urlEncode(credentials.clientSecret);
	const HttpResponse response = request(L"auth.opensky-network.org",
		L"/auth/realms/opensky-network/protocol/openid-connect/token", body,
		L"Content-Type: application/x-www-form-urlencoded\r\n", true);
	const std::string token = jsonString(response.body, "access_token");
	if (token.empty()) {
		error = "OpenSky OAuth HTTP " + std::to_string(response.status) + ": " + response.body.substr(0, 300);
	}
	return token;
}

HttpResponse authenticatedStates(const std::string& token, long long snapshotTime_s) {
	const std::wstring path = L"/api/states/all";
	const std::wstring headers = std::wstring(L"Authorization: Bearer ")
		+ std::wstring(token.begin(), token.end()) + L"\r\n";
	return request(L"opensky-network.org", path.c_str(), {},
		headers.c_str(), false);
}

HttpResponse authenticatedGet(const std::string& token, const std::wstring& path) {
	const std::wstring headers = std::wstring(L"Authorization: Bearer ")
		+ std::wstring(token.begin(), token.end()) + L"\r\n";
	return request(L"opensky-network.org", path.c_str(), {}, headers.c_str(), false);
}

std::vector<std::string> objectArray(const std::string& response) {
	std::vector<std::string> result;
	const std::size_t open = response.find('[');
	if (open == std::string::npos) return result;
	int depth = 0;
	bool quoted = false;
	std::size_t start = std::string::npos;
	for (std::size_t i = open; i < response.size(); ++i) {
		if (response[i] == '\"' && (i == 0 || response[i - 1] != '\\')) quoted = !quoted;
		if (quoted) continue;
		if (response[i] == '{' && depth++ == 0) start = i;
		if (response[i] == '}' && --depth == 0 && start != std::string::npos) {
			result.push_back(response.substr(start, i - start + 1));
			start = std::string::npos;
		}
	}
	return result;
}

}

namespace {
constexpr char cacheMagic[] = "MLSSFL1";
constexpr std::uint32_t maxCachePaths = 100000;
constexpr std::uint32_t maxCachePoints = 10000000;

template <typename T>
bool writeBinary(std::ofstream& file, const T& value) {
	file.write(reinterpret_cast<const char*>(&value), sizeof(value));
	return static_cast<bool>(file);
}

template <typename T>
bool readBinary(std::ifstream& file, T& value) {
	file.read(reinterpret_cast<char*>(&value), sizeof(value));
	return static_cast<bool>(file);
}
}

bool OpenSkyClient::savePaths(const std::string& path,
	const std::vector<std::vector<Aircraft::PathPoint>>& paths, std::string& error) {
	if (paths.size() > maxCachePaths) {
		error = "too many paths for cache";
		return false;
	}
	std::uint64_t totalPoints = 0;
	for (const auto& flight : paths) totalPoints += flight.size();
	if (totalPoints > maxCachePoints) {
		error = "too many path points for cache";
		return false;
	}

	std::ofstream file(path, std::ios::binary | std::ios::trunc);
	if (!file) {
		error = "could not open cache file for writing";
		return false;
	}
	const std::uint32_t pathCount = static_cast<std::uint32_t>(paths.size());
	file.write(cacheMagic, sizeof(cacheMagic) - 1);
	if (!file || !writeBinary(file, pathCount)) {
		error = "could not write cache header";
		return false;
	}
	for (const auto& flight : paths) {
		const std::uint32_t pointCount = static_cast<std::uint32_t>(flight.size());
		if (!writeBinary(file, pointCount)) {
			error = "could not write cache path";
			return false;
		}
		for (const auto& point : flight) {
			if (!writeBinary(file, point.time_s) || !writeBinary(file, point.earthFixedPosition_km.x)
				|| !writeBinary(file, point.earthFixedPosition_km.y)
				|| !writeBinary(file, point.earthFixedPosition_km.z)) {
				error = "could not write cache point";
				return false;
			}
		}
	}
	return true;
}

bool OpenSkyClient::loadPaths(const std::string& path,
	std::vector<std::vector<Aircraft::PathPoint>>& paths, std::string& error) {
	std::ifstream file(path, std::ios::binary);
	if (!file) {
		error = "could not open cache file";
		return false;
	}
	char magic[sizeof(cacheMagic) - 1]{};
	file.read(magic, sizeof(magic));
	if (!file || !std::equal(std::begin(magic), std::end(magic), std::begin(cacheMagic))) {
		error = "invalid flight cache format";
		return false;
	}
	std::uint32_t pathCount = 0;
	if (!readBinary(file, pathCount) || pathCount > maxCachePaths) {
		error = "invalid flight cache path count";
		return false;
	}
	std::vector<std::vector<Aircraft::PathPoint>> loaded;
	loaded.reserve(pathCount);
	std::uint64_t totalPoints = 0;
	for (std::uint32_t i = 0; i < pathCount; ++i) {
		std::uint32_t pointCount = 0;
		if (!readBinary(file, pointCount)) {
			error = "truncated flight cache";
			return false;
		}
		totalPoints += pointCount;
		if (pointCount > maxCachePoints || totalPoints > maxCachePoints) {
			error = "invalid flight cache point count";
			return false;
		}
		std::vector<Aircraft::PathPoint> flight;
		flight.reserve(pointCount);
		for (std::uint32_t j = 0; j < pointCount; ++j) {
			Aircraft::PathPoint point{};
			if (!readBinary(file, point.time_s) || !readBinary(file, point.earthFixedPosition_km.x)
				|| !readBinary(file, point.earthFixedPosition_km.y)
				|| !readBinary(file, point.earthFixedPosition_km.z)) {
				error = "truncated flight cache";
				return false;
			}
			if (!std::isfinite(point.time_s) || !std::isfinite(point.earthFixedPosition_km.x)
				|| !std::isfinite(point.earthFixedPosition_km.y) || !std::isfinite(point.earthFixedPosition_km.z)) {
				error = "invalid value in flight cache";
				return false;
			}
			flight.push_back(point);
		}
		loaded.push_back(std::move(flight));
	}
	paths = std::move(loaded);
	return true;
}

OpenSkyUsageResult OpenSkyClient::checkUsage(const std::string& credentialsJson) {
	OpenSkyUsageResult result;
	Credentials credentials;
	if (!parseCredentials(credentialsJson, credentials)) {
		result.error = "OpenSky credentials must contain clientId and clientSecret";
		return result;
	}
	std::string authenticationError;
	const std::string token = bearerToken(credentials, authenticationError);
	if (token.empty()) {
		result.error = authenticationError.empty() ? "OpenSky OAuth authentication failed" : authenticationError;
		return result;
	}
	result.endpoint = "/api/states/all";
	const HttpResponse response = authenticatedStates(token, 0);
	result.httpStatus = response.status;
	result.rateLimitRemaining = response.rateLimitRemaining;
	result.retryAfterSeconds = response.retryAfterSeconds;
	if (response.status < 200 || response.status >= 300) {
		result.error = "OpenSky usage probe HTTP " + std::to_string(response.status) + ": " + response.body.substr(0, 300);
	}
	return result;
}

namespace {

long long integerField(const std::string& object, const std::string& name) {
	const std::regex pattern("\\\"" + name + "\\\"\\s*:\\s*([0-9]+)");
	std::smatch match;
	if (!std::regex_search(object, match, pattern)) return 0;
	try { return std::stoll(match[1].str()); }
	catch (...) { return 0; }
}

std::string arrayValue(const std::vector<std::string>& values, std::size_t index) {
	return index < values.size() && values[index] != "null" ? values[index] : std::string{};
}

double number(const std::vector<std::string>& values, std::size_t index) {
	const std::string value = arrayValue(values, index);
	if (value.empty()) return std::nan("");
	try { return std::stod(value); }
	catch (...) { return std::nan(""); }
}

std::vector<std::string> splitArray(const std::string& value) {
	std::vector<std::string> result;
	bool quoted = false;
	std::size_t start = 0;
	for (std::size_t i = 0; i < value.size(); ++i) {
		if (value[i] == '\"' && (i == 0 || value[i - 1] != '\\')) quoted = !quoted;
		if (value[i] == ',' && !quoted) {
			result.push_back(value.substr(start, i - start));
			start = i + 1;
		}
	}
	result.push_back(value.substr(start));
	return result;
}

std::string namedArray(const std::string& response, const std::string& name) {
	const std::size_t marker = response.find("\"" + name + "\"");
	if (marker == std::string::npos) return {};
	const std::size_t open = response.find('[', marker);
	if (open == std::string::npos) return {};
	int depth = 0;
	bool quoted = false;
	for (std::size_t i = open; i < response.size(); ++i) {
		if (response[i] == '\"' && (i == 0 || response[i - 1] != '\\')) quoted = !quoted;
		if (quoted) continue;
		if (response[i] == '[') ++depth;
		if (response[i] == ']' && --depth == 0) return response.substr(open + 1, i - open - 1);
	}
	return {};
}

std::string statesArray(const std::string& response) { return namedArray(response, "states"); }

std::vector<std::string> stateArrays(const std::string& states) {
	std::vector<std::string> result;
	int depth = 0;
	std::size_t start = std::string::npos;
	bool quoted = false;
	for (std::size_t i = 0; i < states.size(); ++i) {
		if (states[i] == '\"' && (i == 0 || states[i - 1] != '\\')) quoted = !quoted;
		if (quoted) continue;
		if (states[i] == '[' && depth++ == 0) start = i + 1;
		if (states[i] == ']' && --depth == 0 && start != std::string::npos) {
			result.push_back(states.substr(start, i - start));
			start = std::string::npos;
		}
	}
	return result;
}

OpenSkyImportResult historicalPaths(const std::string& token, long long begin_s,
	double duration_s, double fallbackAltitude_km, int maxAircraft) {
	OpenSkyImportResult result;
	const long long runEnd_s = begin_s + static_cast<long long>(duration_s);
	const long long queryStart_s = begin_s - 7200;
	std::vector<std::string> flightsFound;
	std::set<std::string> seenFlights;
	for (long long cursor_s = queryStart_s; cursor_s < runEnd_s; cursor_s += 7200) {
		const long long end_s = cursor_s + 7200 < runEnd_s ? cursor_s + 7200 : runEnd_s;
		const std::wstring flightsPath = L"/api/flights/all?begin="
			+ std::to_wstring(cursor_s) + L"&end=" + std::to_wstring(end_s);
		const HttpResponse flights = authenticatedGet(token, flightsPath);
		if (!flights.rateLimitRemaining.empty()) result.rateLimitRemaining = flights.rateLimitRemaining;
		if (flights.status == 404) continue;
		if (flights.status == 400 && flights.body.find("30 days") != std::string::npos) {
			result.error = "OpenSky historical tracks are limited to the last 30 days";
			return result;
		}
		if (flights.status < 200 || flights.status >= 300) {
			result.error = "OpenSky flights HTTP " + std::to_string(flights.status) + ": " + flights.body.substr(0, 300);
			if (flights.status == 429 && !flights.retryAfterSeconds.empty()) {
				result.error += " (retry after " + flights.retryAfterSeconds + " seconds)";
			}
			return result;
		}
		for (const auto& flight : objectArray(flights.body)) {
			const std::string icao24 = jsonString(flight, "icao24");
			const long long firstSeen = integerField(flight, "firstSeen");
			const long long lastSeen = integerField(flight, "lastSeen");
			const std::string key = icao24 + ":" + std::to_string(firstSeen);
			if (!icao24.empty() && firstSeen != 0 && lastSeen >= begin_s && firstSeen <= runEnd_s
				&& seenFlights.insert(key).second) flightsFound.push_back(flight);
		}
	}

	int tracksRequested = 0;
	int tracksNotFound = 0;
	int tracksFailed = 0;
	std::string firstTrackFailure;
	for (const auto& flight : flightsFound) {
		if (maxAircraft > 0 && tracksRequested >= maxAircraft) break;
		const std::string icao24 = jsonString(flight, "icao24");
		const long long firstSeen = integerField(flight, "firstSeen");
		const long long lastSeen = integerField(flight, "lastSeen");
		if (icao24.empty() || firstSeen == 0) continue;
		const long long trackTime = firstSeen + (lastSeen > firstSeen ? (lastSeen - firstSeen) / 2 : 0);
		++tracksRequested;
		const std::wstring trackPath = L"/api/tracks/all?icao24="
			+ std::wstring(icao24.begin(), icao24.end()) + L"&time=" + std::to_wstring(trackTime);
		const HttpResponse track = authenticatedGet(token, trackPath);
		if (!track.rateLimitRemaining.empty()) result.rateLimitRemaining = track.rateLimitRemaining;
		if (track.status == 404) {
			++tracksNotFound;
			continue;
		}
		if (track.status < 200 || track.status >= 300) {
			++tracksFailed;
			if (firstTrackFailure.empty()) {
				firstTrackFailure = "HTTP " + std::to_string(track.status) + ": " + track.body.substr(0, 200);
				if (!track.retryAfterSeconds.empty()) {
					firstTrackFailure += " (retry after " + track.retryAfterSeconds + " seconds)";
				}
			}
			continue;
		}
		std::vector<Aircraft::PathPoint> path;
		for (const auto& waypoint : stateArrays(namedArray(track.body, "path"))) {
			const std::vector<std::string> values = splitArray(waypoint);
			const double time_s = number(values, 0) - static_cast<double>(begin_s);
			const double latitude_deg = number(values, 1);
			const double longitude_deg = number(values, 2);
			const double altitude_km = std::isfinite(number(values, 3))
				? number(values, 3) / 1000.0 : fallbackAltitude_km;
			if (std::isfinite(time_s) && std::isfinite(latitude_deg) && std::isfinite(longitude_deg)) {
				path.push_back({time_s, UnitConverter::llaToEcef(UnitConverter::degToRad(longitude_deg),
					UnitConverter::degToRad(latitude_deg), altitude_km)});
			}
		}
		if (path.size() >= 2) result.paths.push_back(std::move(path));
	}
	if (result.paths.empty()) {
		result.error = "OpenSky returned no usable historical tracks (flights="
			+ std::to_string(flightsFound.size()) + ", track_requests=" + std::to_string(tracksRequested)
			+ ", not_found=" + std::to_string(tracksNotFound) + ", failed=" + std::to_string(tracksFailed)
			+ (firstTrackFailure.empty() ? ")" : ", first_failure=" + firstTrackFailure + ")");
	}
	return result;
}
}

OpenSkyImportResult OpenSkyClient::fetchPaths(const std::string& credentialsJson,
	long long snapshotTime_s, double duration_s, double fallbackAltitude_km, int maxAircraft) {
	OpenSkyImportResult result;
	Credentials credentials;
	if (!parseCredentials(credentialsJson, credentials)) {
		result.error = "OpenSky credentials must contain clientId and clientSecret";
		return result;
	}
	if (snapshotTime_s < 0 || snapshotTime_s > 4102444800LL || duration_s <= 0.0
		|| fallbackAltitude_km < 0.0 || maxAircraft < 0) {
		result.error = "OpenSky snapshot time must be Unix seconds, and duration and altitude must be valid";
		return result;
	}

	std::string authenticationError;
	const std::string token = bearerToken(credentials, authenticationError);
	if (token.empty()) {
		result.error = authenticationError.empty() ? "OpenSky OAuth authentication failed" : authenticationError;
		return result;
	}
	if (snapshotTime_s > 0) {
		return historicalPaths(token, snapshotTime_s, duration_s, fallbackAltitude_km, maxAircraft);
	}
	const HttpResponse response = authenticatedStates(token, snapshotTime_s);
	result.rateLimitRemaining = response.rateLimitRemaining;
	if (response.status < 200 || response.status >= 300) {
		result.error = "OpenSky states HTTP " + std::to_string(response.status) + ": " + response.body.substr(0, 300);
		return result;
	}
	const std::vector<std::string> states = stateArrays(statesArray(response.body));
	if (states.empty()) {
		result.error = "OpenSky returned no state vectors";
		return result;
	}

	for (const auto& state : states) {
		if (maxAircraft > 0 && static_cast<int>(result.paths.size()) >= maxAircraft) break;
		const std::vector<std::string> values = splitArray(state);
		const double longitude_deg = number(values, 5);
		const double latitude_deg = number(values, 6);
		const double barometricAltitude_km = number(values, 7) / 1000.0;
		const double velocity_kmps = number(values, 9) / 1000.0;
		const double heading_rad = UnitConverter::degToRad(number(values, 10));
		const double geometricAltitude_km = number(values, 13) / 1000.0;
		const double altitude_km = std::isfinite(geometricAltitude_km) ? geometricAltitude_km
			: (std::isfinite(barometricAltitude_km) ? barometricAltitude_km : fallbackAltitude_km);
		if (!std::isfinite(longitude_deg) || !std::isfinite(latitude_deg) || !std::isfinite(altitude_km)) continue;

		const Vector3d position = UnitConverter::llaToEcef(UnitConverter::degToRad(longitude_deg),
			UnitConverter::degToRad(latitude_deg), altitude_km);
		Vector3d velocity{};
		if (std::isfinite(velocity_kmps) && std::isfinite(heading_rad)) {
			velocity = UnitConverter::headingToVelocity(position, heading_rad, velocity_kmps);
		}
		result.paths.push_back({{0.0, position}, {duration_s, position + velocity * duration_s}});
	}
	if (result.paths.empty()) result.error = "OpenSky returned no usable state vectors";
	return result;
}
