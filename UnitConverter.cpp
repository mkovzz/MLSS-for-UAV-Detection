#include "UnitConverter.h"

#include <algorithm>
#include <cmath>

#include "Earth.h"

/// <summary>
/// Converts degrees to radians.
/// </summary>
/// <param name="degrees">Angle in degrees.</param>
/// <returns>Angle in radians.</returns>
double UnitConverter::degToRad(double degrees) { return degrees * pi / 180.0; }
/// <summary>
/// Converts radians to degrees.
/// </summary>
/// <param name="radians">Angle in radians.</param>
/// <returns>Angle in degrees.</returns>
double UnitConverter::radToDeg(double radians) { return radians * 180.0 / pi; }
/// <summary>
/// Wraps an angle into the [0, 2pi) interval.
/// </summary>
/// <param name="angle">Angle in radians.</param>
/// <returns>Wrapped angle in radians.</returns>
double UnitConverter::wrapTwoPi(double angle) { double result = std::fmod(angle, 2.0 * pi); return result < 0.0 ? result + 2.0 * pi : result; }

/// <summary>
/// Converts geodetic longitude, latitude, and altitude to ECEF coordinates.
/// </summary>
/// <param name="longitude">Longitude in radians.</param>
/// <param name="latitude">Latitude in radians.</param>
/// <param name="altitude">Altitude in kilometers.</param>
/// <returns>ECEF position in kilometers.</returns>
Vector3d UnitConverter::llaToEcef(double longitude, double latitude, double altitude) {
    const double e2 = Earth::flattening * (2.0 - Earth::flattening);
    const double sinLatitude = std::sin(latitude);
    const double cosLatitude = std::cos(latitude);
    const double n = Earth::equatorial_radius / std::sqrt(1.0 - e2 * sinLatitude * sinLatitude);
    return {(n + altitude) * cosLatitude * std::cos(longitude), (n + altitude) * cosLatitude * std::sin(longitude), (n * (1.0 - e2) + altitude) * sinLatitude};
}

/// <summary>
/// Rotates an ECI position into the Earth-fixed frame.
/// </summary>
/// <param name="value">ECI position in kilometers.</param>
/// <param name="time">Elapsed time in seconds.</param>
/// <returns>ECEF position in kilometers.</returns>
Vector3d UnitConverter::eciToEcef(const Vector3d& value, double time) {
    const double angle = Earth::rotation_rate * time;
    const double c = std::cos(angle), s = std::sin(angle);
    return {c * value.x + s * value.y, -s * value.x + c * value.y, value.z};
}

/// <summary>
/// Rotates an Earth-fixed position into the inertial frame.
/// </summary>
/// <param name="value">ECEF position in kilometers.</param>
/// <param name="time">Elapsed time in seconds.</param>
/// <returns>ECI position in kilometers.</returns>
Vector3d UnitConverter::ecefToEci(const Vector3d& value, double time) {
    const double angle = Earth::rotation_rate * time;
    const double c = std::cos(angle), s = std::sin(angle);
    return {c * value.x - s * value.y, s * value.x + c * value.y, value.z};
}

/// <summary>
/// Converts both position and velocity from ECI to ECEF.
/// </summary>
/// <param name="value">ECI body state.</param>
/// <param name="time">Elapsed time in seconds.</param>
/// <returns>ECEF body state.</returns>
BodyState UnitConverter::eciToEcef(const BodyState& value, double time) {
    const Vector3d omegaCrossPosition{-Earth::rotation_rate * value.position.y, Earth::rotation_rate * value.position.x, 0.0};
    return {eciToEcef(value.position, time), eciToEcef(value.velocity - omegaCrossPosition, time)};
}

/// <summary>
/// Converts orbital elements into inertial position and velocity.
/// </summary>
/// <param name="elements">Orbital elements.</param>
/// <returns>ECI body state.</returns>
BodyState UnitConverter::orbitalElementsToState(const OrbitalElements& elements) {
    const double p = elements.a * (1.0 - elements.e * elements.e);
    const double radius = p / (1.0 + elements.e * std::cos(elements.v));
    const double scale = std::sqrt(Earth::mu / p);
    const Vector3d positionPerifocal{radius * std::cos(elements.v), radius * std::sin(elements.v), 0.0};
    const Vector3d velocityPerifocal{-scale * std::sin(elements.v), scale * (elements.e + std::cos(elements.v)), 0.0};
    const double co = std::cos(elements.big_omega), so = std::sin(elements.big_omega);
    const double ci = std::cos(elements.i), si = std::sin(elements.i);
    const double cw = std::cos(elements.w), sw = std::sin(elements.w);
    const auto rotate = [=](const Vector3d& v) { return Vector3d{(co * cw - so * sw * ci) * v.x + (-co * sw - so * cw * ci) * v.y, (so * cw + co * sw * ci) * v.x + (-so * sw + co * cw * ci) * v.y, sw * si * v.x + cw * si * v.y}; };
    return {rotate(positionPerifocal), rotate(velocityPerifocal)};
}

/// <summary>
/// Derives orbital elements from an inertial position and velocity.
/// </summary>
/// <param name="position">ECI position in kilometers.</param>
/// <param name="velocity">ECI velocity in kilometers per second.</param>
/// <returns>Derived orbital elements.</returns>
OrbitalElements UnitConverter::stateToOrbitalElements(const Vector3d& position, const Vector3d& velocity) {
    OrbitalElements result;
    const double radius = position.Magnitude();
    const Vector3d hVector = position.Cross(velocity);
    const double h = hVector.Magnitude();
    if (radius <= 0.0 || h <= 0.0) return result;
    result.a = 1.0 / (2.0 / radius - velocity.Dot(velocity) / Earth::mu);
    result.i = std::acos(std::clamp(hVector.z / h, -1.0, 1.0));
    const Vector3d node{-hVector.y, hVector.x, 0.0};
    const double nodeMagnitude = node.Magnitude();
    const Vector3d eccentricityVector = velocity.Cross(hVector) / Earth::mu - position / radius;
    result.e = eccentricityVector.Magnitude();
    if (nodeMagnitude > 1e-12) result.big_omega = wrapTwoPi(std::atan2(node.y, node.x));
    // Circular orbits have undefined periapsis orientation, so use a stable convention.
    if (result.e < 1e-8) {
        result.e = 0.0;
        result.w = 0.0;
        if (nodeMagnitude > 1e-12) {
            const Vector3d nodeUnit = node / nodeMagnitude;
            const Vector3d orbitUnit = hVector / h;
            const Vector3d perpendicular = orbitUnit.Cross(nodeUnit);
            result.v = wrapTwoPi(std::atan2(position.Dot(perpendicular), position.Dot(nodeUnit)));
        } else {
            result.v = wrapTwoPi(std::atan2(position.y, position.x));
        }
        return result;
    }
    if (nodeMagnitude > 1e-12) {
        result.w = std::acos(std::clamp(node.Dot(eccentricityVector) / (nodeMagnitude * result.e), -1.0, 1.0));
        if (eccentricityVector.z < 0.0) result.w = 2.0 * pi - result.w;
    }
    if (result.e > 1e-12) {
        result.v = std::acos(std::clamp(eccentricityVector.Dot(position) / (result.e * radius), -1.0, 1.0));
        if (position.Dot(velocity) < 0.0) result.v = 2.0 * pi - result.v;
    } else {
        result.v = wrapTwoPi(std::atan2(position.y, position.x));
    }
    return result;
}

/// <summary>
/// Converts a local heading into an inertial velocity vector.
/// </summary>
/// <param name="position">Position defining the local frame.</param>
/// <param name="heading">Heading in radians.</param>
/// <param name="speed">Speed in kilometers per second.</param>
/// <returns>Velocity vector in kilometers per second.</returns>
Vector3d UnitConverter::headingToVelocity(const Vector3d& position, double heading, double speed) {
    const Vector3d up = position.Normalized();
    const Vector3d east = Vector3d{-up.y, up.x, 0.0}.Normalized();
    const Vector3d north = east.Cross(up).Normalized();
    return (north * std::cos(heading) + east * std::sin(heading)) * speed;
}

/// <summary>
/// Computes the circular orbital speed at a radius.
/// </summary>
/// <param name="radius_km">Orbital radius in kilometers.</param>
/// <returns>Circular speed in kilometers per second.</returns>
double UnitConverter::circularOrbitSpeed(double radius_km) { return radius_km > 0.0 ? std::sqrt(Earth::mu / radius_km) : 0.0; }
/// <summary>
/// Converts true anomaly to mean anomaly.
/// </summary>
/// <param name="trueAnomaly">True anomaly in radians.</param>
/// <param name="eccentricity">Orbital eccentricity.</param>
/// <returns>Mean anomaly in radians.</returns>
double UnitConverter::meanAnomalyFromTrue(double trueAnomaly, double eccentricity) { const double eccentricAnomaly = 2.0 * std::atan2(std::sqrt(1.0 - eccentricity) * std::sin(trueAnomaly / 2.0), std::sqrt(1.0 + eccentricity) * std::cos(trueAnomaly / 2.0)); return eccentricAnomaly - eccentricity * std::sin(eccentricAnomaly); }
/// <summary>
/// Converts eccentric anomaly to true anomaly.
/// </summary>
/// <param name="eccentricAnomaly">Eccentric anomaly in radians.</param>
/// <param name="eccentricity">Orbital eccentricity.</param>
/// <returns>True anomaly in radians.</returns>
double UnitConverter::trueAnomalyFromEccentric(double eccentricAnomaly, double eccentricity) { return 2.0 * std::atan2(std::sqrt(1.0 + eccentricity) * std::sin(eccentricAnomaly / 2.0), std::sqrt(1.0 - eccentricity) * std::cos(eccentricAnomaly / 2.0)); }
/// <summary>
/// Converts a source length into kilometers.
/// </summary>
/// <param name="value">Source length value.</param>
/// <param name="meters_per_unit">Meters represented by one source unit.</param>
/// <returns>Length in kilometers.</returns>
double UnitConverter::sourceLengthToKm(double value, double meters_per_unit) { return value * meters_per_unit / 1000.0; }
/// <summary>
/// Converts a source speed into kilometers per second.
/// </summary>
/// <param name="value">Source speed value.</param>
/// <param name="meters_per_second_per_unit">Meters per second represented by one source unit.</param>
/// <returns>Speed in kilometers per second.</returns>
double UnitConverter::sourceSpeedToKmPerS(double value, double meters_per_second_per_unit) { return value * meters_per_second_per_unit / 1000.0; }
