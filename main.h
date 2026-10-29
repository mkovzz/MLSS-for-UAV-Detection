#pragma once
#include <iostream>

#include "Vector3d.h"
#include "orbital_elements.h"
#include "Satellite.h"


/**
* @brief steps the simulation forward by a tenth of a second 
* @param h: step size
* @return The current time
*/
double step_scheduler(int h, double& t);
