#pragma once

#include <string>
#include <vector>

#include "Aircraft.h"

struct OpenSkyImportResult {
	std::vector<std::vector<Aircraft::PathPoint>> paths;
	std::string error;
	std::string rateLimitRemaining;
};

struct OpenSkyUsageResult {
	std::string error;
	std::string endpoint;
	std::string rateLimitRemaining;
	std::string retryAfterSeconds;
	unsigned long httpStatus = 0;
};

class OpenSkyClient {
public:
	/// Fetches a state-vector snapshot and extrapolates each aircraft for duration_s.
	/// Credentials are supplied as the OpenSky client JSON line at runtime.
	static OpenSkyImportResult fetchPaths(const std::string& credentialsJson,
		long long snapshotTime_s, double duration_s, double fallbackAltitude_km, int maxAircraft = 0);
	static bool savePaths(const std::string& path,
		const std::vector<std::vector<Aircraft::PathPoint>>& paths, std::string& error);
	static bool loadPaths(const std::string& path,
		std::vector<std::vector<Aircraft::PathPoint>>& paths, std::string& error);
	static OpenSkyUsageResult checkUsage(const std::string& credentialsJson);
};
