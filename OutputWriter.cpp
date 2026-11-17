#include "OutputWriter.h"

#include <iomanip>

/// <summary>
/// Sets the number of simulation steps between stored samples.
/// </summary>
/// <param name="n">Storage interval in simulation steps.</param>
void OutputWriter::setStorageRate(int n)
{
    storageRate_ = n < 1 ? 1 : n;
}

/// <summary>
/// Records one ephemeris sample for every active entity.
/// </summary>
/// <param name="satellites">Satellites to record.</param>
/// <param name="groundStations">Ground stations to record.</param>
/// <param name="aircraft">Aircraft to record.</param>
void OutputWriter::recordStep(
    const std::vector<std::shared_ptr<SatelliteBody>>& satellites,
    const std::vector<std::shared_ptr<GroundStation>>& groundStations,
    const std::vector<std::shared_ptr<Aircraft>>& aircraft)
{
    for (const auto& satellite : satellites) {
        satellite->recordEphemeris();
    }
    for (const auto& station : groundStations) {
        station->recordEphemeris();
    }
    for (const auto& entity : aircraft) {
        entity->recordEphemeris();
    }
}

/// <summary>
/// Builds a combined table keyed by entity identifier.
/// </summary>
/// <param name="satellites">Satellites to include.</param>
/// <param name="groundStations">Ground stations to include.</param>
/// <param name="aircraft">Aircraft to include.</param>
/// <returns>The combined ephemeris table.</returns>
EphemerisTable OutputWriter::buildTable(
    const std::vector<std::shared_ptr<SatelliteBody>>& satellites,
    const std::vector<std::shared_ptr<GroundStation>>& groundStations,
    const std::vector<std::shared_ptr<Aircraft>>& aircraft) const
{
    EphemerisTable table;
    for (const auto& satellite : satellites) {
        table.entities[satellite->getId()] = satellite->getEphemeris();
    }
    for (const auto& station : groundStations) {
        table.entities[station->getId()] = station->getEphemeris();
    }
    for (const auto& entity : aircraft) {
        table.entities[entity->getId()] = entity->getEphemeris();
    }
    return table;
}

/// <summary>
/// Writes the combined ephemeris table as CSV.
/// </summary>
/// <param name="out">Output stream receiving CSV text.</param>
/// <param name="satellites">Satellites to include.</param>
/// <param name="groundStations">Ground stations to include.</param>
/// <param name="aircraft">Aircraft to include.</param>
void OutputWriter::print(
    std::ostream& out,
    const std::vector<std::shared_ptr<SatelliteBody>>& satellites,
    const std::vector<std::shared_ptr<GroundStation>>& groundStations,
    const std::vector<std::shared_ptr<Aircraft>>& aircraft) const
{
    // Build first so all entity types use the same output ordering and format.
    const EphemerisTable table = buildTable(satellites, groundStations, aircraft);
    out << "entity_id,time_s,x_km,y_km,z_km,vx_kmps,vy_kmps,vz_kmps\n";
    out << std::fixed << std::setprecision(6);
    for (const auto& entity : table.entities) {
        for (const auto& sample : entity.second.samples) {
            out << entity.first << ',' << sample.first << ','
                << sample.second.position.x << ','
                << sample.second.position.y << ','
                << sample.second.position.z << ','
                << sample.second.velocity.x << ','
                << sample.second.velocity.y << ','
                << sample.second.velocity.z << '\n';
        }
    }
}
