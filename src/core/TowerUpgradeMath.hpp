#pragma once

#include "core/Config.hpp"

namespace td {

float upgradedStatValue(float baseValue, const StatUpgradeConfig& upgrade, int level);

// Cost to buy the level after `currentLevel` (i.e. currentLevel -> currentLevel + 1):
// baseCost * costGrowth^currentLevel, rounded, floored at 1.
int upgradeCost(const StatUpgradeConfig& upgrade, int currentLevel);

} // namespace td
