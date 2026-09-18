#include "core/Config.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

namespace td {

namespace {

nlohmann::json loadJson(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open config file: " + path);
    }
    nlohmann::json j;
    try {
        file >> j;
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to parse config file " + path + ": " + e.what());
    }
    return j;
}

} // namespace

GameConfig loadGameConfig(const std::string& dataDir) {
    GameConfig config;

    {
        auto j = loadJson(dataDir + "/tower.json");
        config.tower.name = j.value("name", config.tower.name);
        config.tower.rangeTiles = j.value("range_tiles", config.tower.rangeTiles);
        config.tower.damage = j.value("damage", config.tower.damage);
        config.tower.attacksPerSecond = j.value("attacks_per_second", config.tower.attacksPerSecond);
        config.tower.cost = j.value("cost", config.tower.cost);
    }
    {
        auto j = loadJson(dataDir + "/wall.json");
        config.wall.name = j.value("name", config.wall.name);
        config.wall.cost = j.value("cost", config.wall.cost);
    }
    {
        auto j = loadJson(dataDir + "/enemy.json");
        config.enemy.name = j.value("name", config.enemy.name);
        config.enemy.health = j.value("health", config.enemy.health);
        config.enemy.speedTilesPerSecond = j.value("speed_tiles_per_second", config.enemy.speedTilesPerSecond);
        config.enemy.castleDamage = j.value("castle_damage", config.enemy.castleDamage);
        config.enemy.killReward = j.value("kill_reward", config.enemy.killReward);
    }
    {
        auto j = loadJson(dataDir + "/economy.json");
        config.economy.currencyName = j.value("currency_name", config.economy.currencyName);
        config.economy.startingCurrency = j.value("starting_currency", config.economy.startingCurrency);
        config.economy.killReward = j.value("kill_reward", config.economy.killReward);
    }
    {
        auto j = loadJson(dataDir + "/waves.json");
        config.waves.baseEnemyCount = j.value("base_enemy_count", config.waves.baseEnemyCount);
        config.waves.growthRatePerWave = j.value("growth_rate_per_wave", config.waves.growthRatePerWave);
        config.waves.spawnIntervalSeconds = j.value("spawn_interval_seconds", config.waves.spawnIntervalSeconds);
        config.waves.timeBetweenWavesSeconds =
            j.value("time_between_waves_seconds", config.waves.timeBetweenWavesSeconds);
        config.waves.initialBuildPhaseSeconds =
            j.value("initial_build_phase_seconds", config.waves.initialBuildPhaseSeconds);
    }
    {
        auto j = loadJson(dataDir + "/difficulty.json");
        config.difficulty.waveSizeMultiplier = j.value("wave_size_multiplier", config.difficulty.waveSizeMultiplier);
        config.difficulty.spawnIntervalMultiplier =
            j.value("spawn_interval_multiplier", config.difficulty.spawnIntervalMultiplier);
        config.difficulty.enemyHealthMultiplier =
            j.value("enemy_health_multiplier", config.difficulty.enemyHealthMultiplier);
        config.difficulty.minValue = j.value("min_value", config.difficulty.minValue);
        config.difficulty.maxValue = j.value("max_value", config.difficulty.maxValue);
    }
    {
        auto j = loadJson(dataDir + "/tower_upgrades.json");
        auto loadStat = [&j](const char* key, StatUpgradeConfig& out) {
            if (!j.contains(key)) return;
            const auto& s = j.at(key);
            out.increment = s.value("increment", out.increment);
            out.baseCost = s.value("base_cost", out.baseCost);
            out.costGrowth = s.value("cost_growth", out.costGrowth);
        };
        loadStat("damage", config.towerUpgrades.damage);
        loadStat("attack_speed", config.towerUpgrades.attackSpeed);
        loadStat("range", config.towerUpgrades.range);
    }

    return config;
}

} // namespace td
