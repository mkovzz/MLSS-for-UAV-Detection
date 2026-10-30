#pragma once

/// <summary>
/// 1. Instantiates the config intake, which will instantiate ground stations, the constellation generator -> satellites,
/// Aircraft replay, Health monitor.
///
/// 2. Instantiates the Time step and scheduler
///
/// 3. Instantiates the Output writer
///
/// Earth is a static helper and is never instantiated (read Earth::mu, Earth::j2, etc. directly).
///
/// All the others are instantiated through their singleton static instance and can be referred to through simple dot notation,
/// aside from the individual entity bodies, for which a shared list of object pointers can be obtained through their generators
/// (ConstellationGenerator::Instance->getSatellites(), ConfigIntake::Instance->getGroundStations()).
/// </summary>
/// <returns>0 for success, -1 for failure</returns>
int InstantiateSubsystem();

/// <summary>
/// The main function of the physics engine.
/// The ephemeris table (D-02) is a per entity and per stored time step table of the position and velocity of each entity
/// in the simulation. It queries and controls the main components:
/// TimeStepScheduler supplies the time; each SatelliteBody is updated to that time; AircraftReplay and each GroundStation
/// supply their D-13 positions; HealthMonitor is checked for an abort; OutputWriter records the step at the storage interval.
/// Precondition: the caller (RunController for now) has passed D-01 to ConfigIntake::Instance->ingest().
/// </summary>
/// <returns>0 for success, -1 for failure</returns>
int ProduceEphemerisTable();