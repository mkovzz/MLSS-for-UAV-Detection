#include "UnitConverter.h"

#include <algorithm>
#include <cmath>

#include "Earth.h"

double UnitConverter::degToRad(double degrees) {
	return degrees * pi / 180.0;
}

double UnitConverter::radToDeg(double radians) {
	return radians * 180.0 / pi;
}

double UnitConverter::wrapTwoPi(double angle) {
	angle = std::fmod(angle, 2.0 * pi);
	if (angle < 0.0) {
		angle += 2.0 * pi;
	}
	return angle;
}

Vector3d UnitConverter::llaToEcef(double longitude, double latitude, double altitude) {
	const double a = Earth::equatorial_radius;
	const double f = Earth::flattening;
	const double e2 = f * (2.0 - f);
	const double sin_lat = std::sin(latitude);
	const double cos_lat = std::cos(latitude);
	const double n = a / std::sqrt(1.0 - e2 * sin_lat * sin_lat);

	return {
		(n + altitude) * cos_lat * std::cos(longitude),
		(n + altitude) * cos_lat * std::sin(longitude),
		(n * (1.0 - e2) + altitude) * sin_lat
	};
}

Vector3d UnitConverter::eciToEcef(const Vector3d& eci, double t_seconds) {
	const double theta = Earth::rotation_rate * t_seconds;
	const double c = std::cos(theta);
	const double s = std::sin(theta);
	return { c * eci.x + s * eci.y, -s * eci.x + c * eci.y, eci.z };
}

Vector3d UnitConverter::ecefToEci(const Vector3d& ecef, double t_seconds) {
	const double theta = Earth::rotation_rate * t_seconds;
	const double c = std::cos(theta);
	const double s = std::sin(theta);
	return { c * ecef.x - s * ecef.y, s * ecef.x + c * ecef.y, ecef.z };
}

BodyState UnitConverter::eciToEcef(const BodyState& eci, double t_seconds) {
	const double theta = Earth::rotation_rate * t_seconds;
	const double c = std::cos(theta);
	const double s = std::sin(theta);

	BodyState ecef;
	ecef.position = { c * eci.position.x + s * eci.position.y, -s * eci.position.x + c * eci.position.y, eci.position.z };

	const Vector3d rotated_v{
		c * eci.velocity.x + s * eci.velocity.y,
		-s * eci.velocity.x + c * eci.velocity.y,
		eci.velocity.z
	};
	// Added: account for Earth rotation when converting inertial velocity into ECEF
	const Vector3d omega{ 0.0, 0.0, Earth::rotation_rate };
	ecef.velocity = rotated_v - omega.Cross(ecef.position);
	return ecef;
}

BodyState UnitConverter::orbitalElementsToState(const OrbitalElements& elements) {
	const double mu = Earth::mu;
	const double a = elements.a;
	const double e = elements.e;
	const double i = elements.i;
	const double O = elements.big_omega;
	const double w = elements.w;
	const double nu = elements.v;

	const double p = a * (1.0 - e * e);
	const double r = p / (1.0 + e * std::cos(nu));
	const Vector3d r_pqw{ r * std::cos(nu), r * std::sin(nu), 0.0 };
	const double sqrt_mu_p = std::sqrt(mu / p);
	const Vector3d v_pqw{ -sqrt_mu_p * std::sin(nu), sqrt_mu_p * (e + std::cos(nu)), 0.0 };

	const double cO = std::cos(O);
	const double sO = std::sin(O);
	const double ci = std::cos(i);
	const double si = std::sin(i);
	const double cw = std::cos(w);
	const double sw = std::sin(w);

	const double R11 = cO * cw - sO * sw * ci;
	const double R12 = -cO * sw - sO * cw * ci;
	const double R13 = sO * si;
	const double R21 = sO * cw + cO * sw * ci;
	const double R22 = -sO * sw + cO * cw * ci;
	const double R23 = -cO * si;
	const double R31 = sw * si;
	const double R32 = cw * si;
	const double R33 = ci;

	BodyState state;
	state.position = {
		R11 * r_pqw.x + R12 * r_pqw.y + R13 * r_pqw.z,
		R21 * r_pqw.x + R22 * r_pqw.y + R23 * r_pqw.z,
		R31 * r_pqw.x + R32 * r_pqw.y + R33 * r_pqw.z
	};
	state.velocity = {
		R11 * v_pqw.x + R12 * v_pqw.y + R13 * v_pqw.z,
		R21 * v_pqw.x + R22 * v_pqw.y + R23 * v_pqw.z,
		R31 * v_pqw.x + R32 * v_pqw.y + R33 * v_pqw.z
	};
	return state;
}

OrbitalElements UnitConverter::stateToOrbitalElements(const Vector3d& position, const Vector3d& velocity) {
	const double mu = Earth::mu;
	const double rmag = position.Magnitude();
	const double vmag = velocity.Magnitude();
	const Vector3d h = position.Cross(velocity);
	const double hmag = h.Magnitude();
	const Vector3d nvec = Vector3d{ 0.0, 0.0, 1.0 }.Cross(h);
	const double nmag = nvec.Magnitude();
	const Vector3d evec = (position * (vmag * vmag - mu / rmag) - velocity * position.Dot(velocity)) / mu;
	const double e = evec.Magnitude();
	const double energy = vmag * vmag / 2.0 - mu / rmag;

	OrbitalElements el;
	el.a = -mu / (2.0 * energy);
	el.e = e;
	el.i = std::acos(std::clamp(h.z / hmag, -1.0, 1.0));

	constexpr double kEps = 1e-12;
	if (nmag > kEps) {
		el.big_omega = std::acos(std::clamp(nvec.x / nmag, -1.0, 1.0));
		if (nvec.y < 0.0) {
			el.big_omega = 2.0 * pi - el.big_omega;
		}
	}
	else {
		el.big_omega = 0.0;
	}

	if (e > 1e-8) {
		if (nmag > kEps) {
			el.w = std::acos(std::clamp(nvec.Dot(evec) / (nmag * e), -1.0, 1.0));
			if (evec.z < 0.0) {
				el.w = 2.0 * pi - el.w;
			}
		}
		else {
			el.w = std::atan2(evec.y, evec.x);
			if (el.w < 0.0) {
				el.w += 2.0 * pi;
			}
		}
		el.v = std::acos(std::clamp(evec.Dot(position) / (e * rmag), -1.0, 1.0));
		if (position.Dot(velocity) < 0.0) {
			el.v = 2.0 * pi - el.v;
		}
	}
	else {
		el.e = 0.0;
		el.w = 0.0;
		if (nmag > kEps) {
			double arg_lat = std::acos(std::clamp(nvec.Dot(position) / (nmag * rmag), -1.0, 1.0));
			if (position.z < 0.0) {
				arg_lat = 2.0 * pi - arg_lat;
			}
			el.v = arg_lat;
		}
		else {
			el.v = std::atan2(position.y, position.x);
			if (el.v < 0.0) {
				el.v += 2.0 * pi;
			}
		}
	}
	return el;
}

Vector3d UnitConverter::headingToVelocity(const Vector3d& position, double heading, double speed) {
	const double r = position.Magnitude();
	const double lon = std::atan2(position.y, position.x);
	const double lat = std::asin(std::clamp(position.z / r, -1.0, 1.0));
	const Vector3d east{ -std::sin(lon), std::cos(lon), 0.0 };
	const Vector3d north{
		-std::sin(lat) * std::cos(lon),
		-std::sin(lat) * std::sin(lon),
		std::cos(lat)
	};
	Vector3d dir = north * std::cos(heading) + east * std::sin(heading);
	const double mag = dir.Magnitude();
	if (mag > 0.0) {
		dir = dir / mag;
	}
	return dir * speed;
}

double UnitConverter::circularOrbitSpeed(double radius_km) {
	return std::sqrt(Earth::mu / radius_km);
}

double UnitConverter::meanAnomalyFromTrue(double true_anomaly, double eccentricity) {
	if (eccentricity < 1e-12) {
		return wrapTwoPi(true_anomaly);
	}
	const double sqrt_1me2 = std::sqrt(1.0 - eccentricity * eccentricity);
	const double E = std::atan2(
		sqrt_1me2 * std::sin(true_anomaly),
		eccentricity + std::cos(true_anomaly)
	);
	return wrapTwoPi(E - eccentricity * std::sin(E));
}

double UnitConverter::trueAnomalyFromEccentric(double eccentric_anomaly, double eccentricity) {
	if (eccentricity < 1e-12) {
		return wrapTwoPi(eccentric_anomaly);
	}
	const double sinE = std::sin(eccentric_anomaly);
	const double cosE = std::cos(eccentric_anomaly);
	const double denom = 1.0 - eccentricity * cosE;
	const double sinv = (std::sqrt(1.0 - eccentricity * eccentricity) * sinE) / denom;
	const double cosv = (cosE - eccentricity) / denom;
	return wrapTwoPi(std::atan2(sinv, cosv));
}

double UnitConverter::sourceLengthToKm(double value, double meters_per_unit) {
	return value * meters_per_unit / 1000.0;
}

double UnitConverter::sourceSpeedToKmPerS(double value, double meters_per_second_per_unit) {
	return value * meters_per_second_per_unit / 1000.0;
}
