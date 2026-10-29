#pragma once

struct OrbitalElements {

	double a = 0.0;			//semi major axis or size of the orbit: half the length of the elliptical's longest path, radius if circle
	double e = 0.0;			//eccentricity: how much orbit deviates from being a perfect circle
	double i = 0.0;			//inclination: tilt of orbital plane w.r.t. equatorial plane (0 = equator, 90 = pole orbit)
	//longitude of ascending node: ascending node is where the orbital plane crosses the equatorial plane orbiting from south to north
	//the angle this crossing point makes with some reference direction is the longitude of the ascending node
	//this variable assumes to be the right ascension of ascending node
	double big_omega = 0.0;
	double w = 0.0;			//argument of perigee: angle between ascending node and perigee (where orbit is closest to planet)
	double v = 0.0;			//true anomaly: angle between the perigee and the orbiting object's current position

};