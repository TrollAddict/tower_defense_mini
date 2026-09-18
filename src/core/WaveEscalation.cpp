#include "core/WaveEscalation.hpp"

#include <algorithm>
#include <cmath>

namespace td {

int nextWaveEnemyCount(int previousCount, float growthRatePerWave) {
    const int grown = static_cast<int>(std::floor(static_cast<float>(previousCount) * (1.0f + growthRatePerWave)));
    return std::max(previousCount + 1, grown);
}

float escalatedEnemyHealth(float baseHealth, float growthRatePerWave, int waveNumber) {
    const int exponent = std::max(0, waveNumber - 1);
    return baseHealth * std::pow(1.0f + growthRatePerWave, static_cast<float>(exponent));
}

} // namespace td
