#include <iostream>
#include <vector>

#include "physics_engine_main.h"
#include "AircraftReplay.h"
#include "ConfigIntake.h"
#include "ConstellationGenerator.h"
#include "EciState.h"
#include "HealthMonitor.h"
#include "OutputWriter.h"
#include "TimeStepScheduler.h"

int InstantiateSubsystem() {

	// Create health monitor
	HealthMonitor::Initialize();
	if (HealthMonitor::Instance == nullptr) {
		std::cerr << "Error: HealthMonitor::Initialize() failed." << std::endl;
		return -1;
	}

	// Create constellation generator
	ConstellationGenerator::Initialize();
	if (ConstellationGenerator::Instance == nullptr) {
		std::cerr << "Error: ConstellationGenerator::Initialize() failed." << std::endl;
		return -1;
	}

	// Create aircraft replay
	AircraftReplay::Initialize();
	if (AircraftReplay::Instance == nullptr) {
		std::cerr << "Error: AircraftReplay::Initialize() failed." << std::endl;
		return -1;
	}

	// Create config intake
	ConfigIntake::Initialize();
	if (ConfigIntake::Instance == nullptr) {
		std::cerr << "Error: ConfigIntake::Initialize() failed." << std::endl;
		return -1;
	}

	//Create time and step scheduler 
	TimeStepScheduler::Initialize();
	if (TimeStepScheduler::Instance == nullptr) {
		std::cerr << "Error: TimeStepScheduler::Initialize() failed." << std::endl;
		return -1;
	}

	//Create output writer
	OutputWriter::Initialize();
	if (OutputWriter::Instance == nullptr) {
		std::cerr << "Error: OutputWriter::Initialize() failed." << std::endl;
		return -1;
	}

	//everything else is either a fully static helper class ( Earth) or is created later from D-01 by the singletons above (SatelliteBody by ConstellationGenerator, GroundStation by ConfigIntake).

	// if instantiations sucessful then return 0
	return 0;
}

int ShutdownSubsystem() {

	// destroy in the reverse order of InstantiateSubsystem(); Shutdown() does nothing for a singleton that does not exist
	OutputWriter::Shutdown();
	TimeStepScheduler::Shutdown();
	ConfigIntake::Shutdown();
	AircraftReplay::Shutdown();
	ConstellationGenerator::Shutdown();
	HealthMonitor::Shutdown();

	return 0;
}

int ProduceEphemerisTable() {
	//This is run when everything is set up, by this point run controller should have given D-01 to ConfigIntake
	//along with setting up setting up all timing parameters in time and step sceduler (start, end, step size, storage interval)

	//null checks 
	if (ConfigIntake::Instance == nullptr || ConstellationGenerator::Instance == nullptr
		|| AircraftReplay::Instance == nullptr || HealthMonitor::Instance == nullptr
		|| TimeStepScheduler::Instance == nullptr || OutputWriter::Instance == nullptr) {
		std::cerr << "Error: ProduceEphemerisTable() called before InstantiateSubsystem() succeeded." << std::endl;
		return -1;
	}

	//Config Intake checked for early abort, if so return -1
	if (HealthMonitor::Instance->isAbortRequested()) {
		std::cerr << "Error: configuration rejected: " << HealthMonitor::Instance->getStatus() << std::endl;
		return -1;
	}

	// pass the storage interval to the output writer, so it knows when to write out the next step
	OutputWriter::Instance->setStorageRate(TimeStepScheduler::Instance->getStorageRate());


	//loop through the time steps, for each step:
	bool running = true;
	while (running) {

		//check health monitor for un-recoverable state, if so return -2 (different error codes will be helpful later)
		if (HealthMonitor::Instance->isAbortRequested()) {
			std::cerr << "Error: run aborted: " << HealthMonitor::Instance->getStatus() << std::endl;
			return -2;
		}

		//Process time step
		const double t = TimeStepScheduler::Instance->getTime();

		//Prompt scheduler to advance


		// each time step, the following will happen:
		//	  a. Tell every satellite (shared list from Constellation Generator) to update to t.
		//       Each satellite reports its own state and any rejection reason to Health Monitor as it does this.
		//       Collect their states.
		for (const auto& satellite : ConstellationGenerator::Instance->getSatellites()) {
			satellite->update(t);
		}
		// 
		//    b. Collect each ground station's fixed position (shared list from Config Intake) and stamp it with t.
		for (const auto& station : ConfigIntake::Instance->getGroundStations()) {
			station->update(t);
		}
		// 
		//    c. Ask Aircraft Replay for all aircraft positions at t.
		for (const auto& aircraft : AircraftReplay::Instance->getAircraft()) {
			aircraft->update(t);
		}
		// 
		//    d. Ask Health Monitor whether anything has requested an abort.
		//       If so, the step stops here and nothing is recorded for it.
		if (HealthMonitor::Instance->isAbortRequested()) {
			std::cerr << "Error: run aborted: " << HealthMonitor::Instance->getStatus() << std::endl;
			return -2;
		}
		// 
		//    e. Otherwise give Output Writer the satellite states, ground-station positions and aircraft positions.
		//       Output Writer converts the Earth-fixed ones into the satellites' frame and keeps the step
		//       only if t lands on a storage-interval boundary.
		OutputWriter::Instance->recordStep(ConstellationGenerator::Instance->getSatellites(),
			ConfigIntake::Instance->getGroundStations(), AircraftReplay::Instance->getAircraft());

		// advance (see "Prompt scheduler to advance" above); it runs once the step above has been processed
		running = TimeStepScheduler::Instance->advance();
	}

	//if no abort signal was fired return 0 to indicate success


	// print the ephemeris table (for now, the only output)
	OutputWriter::Instance->print(std::cout, ConstellationGenerator::Instance->getSatellites(),
		ConfigIntake::Instance->getGroundStations(), AircraftReplay::Instance->getAircraft());

	return 0;
}