#include "core/WaveEscalation.hpp"

#include <algorithm>
#include <cmath>

namespace td {

int nextWaveEnemyCount(int previousCount, float growthRatePerWave) {
    const int grown = static_cast<int>(std::floor(static_cast<float>(previousCount) * (1.0f + growthRatePerWave)));
    return std::max(previousCount + 1, grown);
}

} // namespace td
