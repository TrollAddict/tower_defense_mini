#include "core/TowerUpgradeMath.hpp"

#include <algorithm>
#include <cmath>

namespace td {

float upgradedStatValue(float baseValue, const StatUpgradeConfig& upgrade, int level) {
    return baseValue + upgrade.increment * static_cast<float>(level);
}

int upgradeCost(const StatUpgradeConfig& upgrade, int currentLevel) {
    const float cost =
        static_cast<float>(upgrade.baseCost) * std::pow(upgrade.costGrowth, static_cast<float>(currentLevel));
    return std::max(1, static_cast<int>(std::lround(cost)));
}

} // namespace td
