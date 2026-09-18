#pragma once

#include <string>

namespace td {

struct TowerConfig {
    std::string name;
    float rangeTiles = 2.0f;
    float damage = 2.0f;
    float attacksPerSecond = 1.0f;
    int cost = 5;
};

struct WallConfig {
    std::string name;
    int cost = 3;
};

struct EnemyConfig {
    std::string name;
    float health = 10.0f;
    float speedTilesPerSecond = 1.5f;
    float castleDamage = 1.0f;
    int killReward = 1;
};

struct EconomyConfig {
    std::string currencyName = "bone";
    int startingCurrency = 10;
    int killReward = 1;
};

struct WaveConfig {
    int baseEnemyCount = 5;
    float growthRatePerWave = 0.10f;
    float spawnIntervalSeconds = 0.75f;
    float timeBetweenWavesSeconds = 6.0f;
    float initialBuildPhaseSeconds = 30.0f;
    float enemyHealthGrowthRatePerWave = 0.15f;
};

struct DifficultyConfig {
    int waveSizeMultiplier = 1;
    int spawnIntervalMultiplier = 1;
    int enemyHealthMultiplier = 1;
    int minValue = 1;
    int maxValue = 99;
};

// A single purchasable stat track: each level adds `increment` to the base stat and
// costs baseCost * costGrowth^currentLevel (rounded), so cost escalates the more it's
// already been bought.
struct StatUpgradeConfig {
    float increment = 0.0f;
    int baseCost = 1;
    float costGrowth = 1.5f;
};

struct TowerUpgradesConfig {
    StatUpgradeConfig damage{1.0f, 8, 1.5f};
    StatUpgradeConfig attackSpeed{0.25f, 8, 1.5f};
    StatUpgradeConfig range{0.5f, 10, 1.5f};
};

struct GameConfig {
    TowerConfig tower;
    WallConfig wall;
    EnemyConfig enemy;
    EconomyConfig economy;
    WaveConfig waves;
    DifficultyConfig difficulty;
    TowerUpgradesConfig towerUpgrades;
};

// Loads every data/*.json file that backs the GDD's "config, not hardcoded" rule
// (GDD §11). Throws std::runtime_error with the offending path if a file is missing
// or malformed — this is a startup-time hard failure, not something to recover from.
GameConfig loadGameConfig(const std::string& dataDir);

} // namespace td
