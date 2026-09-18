// Coverage for the global tower stat upgrade math (core/TowerUpgradeMath). Kept as a
// pure-function test, same reasoning as flow_field_test.cpp and
// wave_escalation_test.cpp: no framework dependency, no need to spin up
// TowerUpgrades/GameState/entt just to check arithmetic.
#include <iostream>

#include "core/TowerUpgradeMath.hpp"

namespace {

int failures = 0;

void check(bool condition, const char* description) {
    if (!condition) {
        std::cerr << "FAILED: " << description << "\n";
        ++failures;
    } else {
        std::cout << "ok: " << description << "\n";
    }
}

void testStatValueScalesLinearlyWithLevel() {
    const td::StatUpgradeConfig upgrade{1.0f, 8, 1.5f};
    check(td::upgradedStatValue(2.0f, upgrade, 0) == 2.0f, "stat value: level 0 equals the unmodified base value");
    check(td::upgradedStatValue(2.0f, upgrade, 3) == 5.0f, "stat value: level 3 adds 3x the increment to the base");
}

void testCostGrowsMultiplicativelyPerLevel() {
    const td::StatUpgradeConfig upgrade{1.0f, 8, 1.5f};
    check(td::upgradeCost(upgrade, 0) == 8, "cost: level 0 -> 1 costs exactly baseCost");
    check(td::upgradeCost(upgrade, 1) == 12, "cost: level 1 -> 2 costs round(8 * 1.5) == 12");
    check(td::upgradeCost(upgrade, 2) == 18, "cost: level 2 -> 3 costs round(8 * 1.5^2) == 18");
}

void testCostNeverDropsBelowOne() {
    const td::StatUpgradeConfig freeUpgrade{1.0f, 0, 1.0f};
    check(td::upgradeCost(freeUpgrade, 0) >= 1, "cost: a misconfigured zero base cost still floors at 1");
}

} // namespace

int main() {
    testStatValueScalesLinearlyWithLevel();
    testCostGrowsMultiplicativelyPerLevel();
    testCostNeverDropsBelowOne();

    if (failures > 0) {
        std::cerr << failures << " check(s) failed\n";
        return 1;
    }
    std::cout << "All checks passed\n";
    return 0;
}
