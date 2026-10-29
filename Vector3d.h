#pragma once
#include <cmath>
#include <ostream>

//Custom Vector3d struct for tracking satellite's position/velocity
struct Vector3d {

	double x = 0.0;
	double y = 0.0;
	double z = 0.0;
	
	//operators for vector arithmetic
	constexpr Vector3d operator+(const Vector3d& other) const {
		return { x + other.x, y + other.y, z + other.z };
	}
	constexpr Vector3d operator-(const Vector3d& other) const {
		return { x - other.x, y - other.y, z - other.z };
	}

	//negation
	constexpr Vector3d operator-() const {
		return { -x, -y, -z };
	}

	//scalar multiplication/division
	constexpr Vector3d operator*(double s) const {
		return { x * s, y * s, z * s };
	}
	constexpr Vector3d operator/(double s) const {
		return { x / s, y / s, z / s };
	}

	//dot and cross products
	constexpr double Dot(const Vector3d& other) const {
		return (x * other.x) + (y * other.y) + (z * other.z);
	}
	constexpr Vector3d Cross(const Vector3d& other) const {
		return { (y * other.z) - (z * other.y), (z * other.x) - (x * other.z), (x * other.y) - (y * other.x) };
	}

	// Added: magnitude helpers used by UnitConverter and closest-station search
	double Magnitude() const {
		return std::sqrt(x * x + y * y + z * z);
	}
	Vector3d Normalized() const {
		const double m = Magnitude();
		if (m == 0.0) {
			return {};
		}
		return *this / m;
	}
};

// Added: allow `scalar * vector` as well as `vector * scalar`
constexpr Vector3d operator*(double s, const Vector3d& v) {
	return v * s;
}

// Added: so Satellite streaming and terminal logs can print vectors
inline std::ostream& operator<<(std::ostream& os, const Vector3d& v) {
	os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
	return os;
}
