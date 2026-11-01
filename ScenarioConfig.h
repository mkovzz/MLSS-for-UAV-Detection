#pragma once

#include <string>

/// <summary>
/// D-01 Scenario configuration. JSON inputs: satellites, aircraft, ground stations, run settings, sensor/radio parameters.
/// Held as the JSON text; ConfigIntake is responsible for reading it. Sensor/radio parameters are ignored by the physics engine.
/// </summary>
struct ScenarioConfig {
    /// The D-01 JSON document.
    std::string json;
};