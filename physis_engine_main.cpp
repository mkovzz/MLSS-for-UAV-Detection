
Physics engine main · CPP
#include <iostream>
#include <vector>

#include "physics_engine_main.h"
#include "AircraftReplay.h"
#include "ConfigIntake.h"
#include "ConstellationGenerator.h"
#include "EarthFixedPosition.h"
#include "EciState.h"
#include "HealthMonitor.h"
#include "OutputWriter.h"
#include "TimeStepScheduler.h"

int InstantiateSubsystem() {

	// Create config intake
	if (ConfigIntake::Instance->instantiate() != 0) {
		std::cerr << "Error: ConfigIntake::Instance->instantiate() failed." << std::endl;
		return -1;
	}

	//Create time and step scheduler 
	if (TimeStepScheduler::Instance->instantiate() != 0) {
		std::cerr << "Error: TimeStepScheduler::Instance->instantiate() failed." << std::endl;
		return -1;
	}

	//Create output writer
	if (OutputWriter::Instance->instantiate() != 0) {
		std::cerr << "Error: OutputWriter::Instance->instantiate() failed." << std::endl;
		return -1;
	}

	//everything else is either a fully static helper class ( Earth) or is instantiated by the above three (ConstellationGenerator, AircraftReplay, HealthMonitor, GroundStation).

	// if instantiations sucessful then return 0
	return 0;
}

int ProduceEphemerisTable() {
	//This is run when everything is set up, by this point run controller should have given D-01 to ConfigIntake
	//along with setting up setting up all timing parameters in time and step sceduler (start, end, step size, storage interval)

	//null checks 

	//Config Intake checked for early abort, if so return -1
	
	// pass the storage interval to the output writer, so it knows when to write out the next step


	//loop through the time steps, for each step:
		
		//check health monitor for un-recoverable state, if so return -2 (different error codes will be helpful later)
		
	    //Process time step
		
		//Prompt scheduler to advance

	
		// each time step, the following will happen:
		//	  a. Tell every satellite (shared list from Constellation Generator) to update to t.
		//       Each satellite reports its own state and any rejection reason to Health Monitor as it does this.
		//       Collect their states.
		// 
		//    b. Collect each ground station's fixed position (shared list from Config Intake) and stamp it with t.
		// 
		//    c. Ask Aircraft Replay for all aircraft positions at t.
		// 
		//    d. Ask Health Monitor whether anything has requested an abort.
		//       If so, the step stops here and nothing is recorded for it.
		// 
		//    e. Otherwise give Output Writer the satellite states, ground-station positions and aircraft positions.
		//       Output Writer converts the Earth-fixed ones into the satellites' frame and keeps the step
		//       only if t lands on a storage-interval boundary.

	//if no abort signal was fired return 0 to indicate success

	
	return 0;
}