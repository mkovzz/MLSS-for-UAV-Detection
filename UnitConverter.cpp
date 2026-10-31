#include "UnitConverter.h"

#include <algorithm>
#include <cmath>

#include "Earth.h"

double UnitConverter::degToRad(double degrees) { return degrees * pi / 180.0; }
double UnitConverter::radToDeg(double radians) { return radians * 180.0 / pi; }
double UnitConverter::wrapTwoPi(double angle) { double result = std::fmod(angle, 2.0 * pi); return result < 0.0 ? result + 2.0 * pi : result; }

Vector3d UnitConverter::llaToEcef(double longitude, double latitude, double altitude) {
    const double e2 = Earth::flattening * (2.0 - Earth::flattening);
    const double sinLatitude = std::sin(latitude);
    const double cosLatitude = std::cos(latitude);
    const double n = Earth::equatorial_radius / std::sqrt(1.0 - e2 * sinLatitude * sinLatitude);
    return {(n + altitude) * cosLatitude * std::cos(longitude), (n + altitude) * cosLatitude * std::sin(longitude), (n * (1.0 - e2) + altitude) * sinLatitude};
}

Vector3d UnitConverter::eciToEcef(const Vector3d& value, double time) {
    const double angle = Earth::rotation_rate * time;
    const double c = std::cos(angle), s = std::sin(angle);
    return {c * value.x + s * value.y, -s * value.x + c * value.y, value.z};
}

Vector3d UnitConverter::ecefToEci(const Vector3d& value, double time) {
    const double angle = Earth::rotation_rate * time;
    const double c = std::cos(angle), s = std::sin(angle);
    return {c * value.x - s * value.y, s * value.x + c * value.y, value.z};
}

BodyState UnitConverter::eciToEcef(const BodyState& value, double time) {
    const Vector3d omegaCrossPosition{-Earth::rotation_rate * value.position.y, Earth::rotation_rate * value.position.x, 0.0};
    return {eciToEcef(value.position, time), eciToEcef(value.velocity - omegaCrossPosition, time)};
}

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

Vector3d UnitConverter::headingToVelocity(const Vector3d& position, double heading, double speed) {
    const Vector3d up = position.Normalized();
    const Vector3d east = Vector3d{-up.y, up.x, 0.0}.Normalized();
    const Vector3d north = east.Cross(up).Normalized();
    return (north * std::cos(heading) + east * std::sin(heading)) * speed;
}

double UnitConverter::circularOrbitSpeed(double radius_km) { return radius_km > 0.0 ? std::sqrt(Earth::mu / radius_km) : 0.0; }
double UnitConverter::meanAnomalyFromTrue(double trueAnomaly, double eccentricity) { const double eccentricAnomaly = 2.0 * std::atan2(std::sqrt(1.0 - eccentricity) * std::sin(trueAnomaly / 2.0), std::sqrt(1.0 + eccentricity) * std::cos(trueAnomaly / 2.0)); return eccentricAnomaly - eccentricity * std::sin(eccentricAnomaly); }
double UnitConverter::trueAnomalyFromEccentric(double eccentricAnomaly, double eccentricity) { return 2.0 * std::atan2(std::sqrt(1.0 + eccentricity) * std::sin(eccentricAnomaly / 2.0), std::sqrt(1.0 - eccentricity) * std::cos(eccentricAnomaly / 2.0)); }
double UnitConverter::sourceLengthToKm(double value, double meters_per_unit) { return value * meters_per_unit / 1000.0; }
double UnitConverter::sourceSpeedToKmPerS(double value, double meters_per_second_per_unit) { return value * meters_per_second_per_unit / 1000.0; }
