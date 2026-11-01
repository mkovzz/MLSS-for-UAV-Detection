#include "OutputWriter.h"

#include <iomanip>

void OutputWriter::setStorageRate(int n)
{
    storageRate_ = n < 1 ? 1 : n;
    stepCount_ = 0;
}

void OutputWriter::recordStep(
    const std::vector<std::shared_ptr<SatelliteBody>>& satellites,
    const std::vector<std::shared_ptr<GroundStation>>& groundStations,
    const std::vector<std::shared_ptr<Aircraft>>& aircraft)
{
    ++stepCount_;
    if (stepCount_ != 1 && stepCount_ % storageRate_ != 0) {
        return;
    }

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

void OutputWriter::print(
    std::ostream& out,
    const std::vector<std::shared_ptr<SatelliteBody>>& satellites,
    const std::vector<std::shared_ptr<GroundStation>>& groundStations,
    const std::vector<std::shared_ptr<Aircraft>>& aircraft) const
{
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
