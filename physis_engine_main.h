#pragma once

/// <summary>
/// Creates every singleton of the physics engine, in dependency order (components that report come after what they report to):
///
/// 1. Health monitor
///
/// 2. Constellation generator
///
/// 3. Aircraft replay
///
/// 4. Config intake
///
/// 5. Time step and scheduler
///
/// 6. Output writer
///
/// Earth is a static helper and is never instantiated (read Earth::mu, Earth::j2, etc. directly).
///
/// The individual entity bodies are created later, from D-01: SatelliteBody by ConstellationGenerator and GroundStation by ConfigIntake.
/// Their shared lists are reached through ConstellationGenerator::Instance->getSatellites() and ConfigIntake::Instance->getGroundStations().
/// </summary>
/// <returns>0 for success, -1 for failure</returns>
int InstantiateSubsystem();

/// <summary>
/// Destroys everything InstantiateSubsystem() created, in reverse order. Safe to call if InstantiateSubsystem() failed part-way
/// or was never called (Shutdown() on a missing singleton does nothing).
/// </summary>
/// <returns>0 for success, -1 for failure</returns>
int ShutdownSubsystem();

/// <summary>
/// The main function of the physics engine.
/// The ephemeris table (D-02) is a per entity and per stored time step table of the position and velocity of each entity
/// in the simulation. It queries and controls the main components:
/// TimeStepScheduler supplies the time; each SatelliteBody is updated to that time; AircraftReplay and each GroundStation
/// supply their D-13 positions; HealthMonitor is checked for an abort; OutputWriter records the step at the storage interval.
/// Precondition: the caller (RunController for now) has passed D-01 to ConfigIntake::Instance->ingest().
/// </summary>
/// <returns>0 for success, -1 if not ready or the configuration was rejected early, -2 if the Health Monitor aborted the run</returns>
int ProduceEphemerisTable();