#include "game/TowerUpgrades.hpp"

#include "core/TowerUpgradeMath.hpp"

namespace td {

namespace {
constexpr float kNotificationSeconds = 2.5f;
}

TowerUpgrades::TowerUpgrades(const GameConfig& config) : config_(config) {}

const StatUpgradeConfig& TowerUpgrades::upgradeConfigFor(UpgradeStat stat) const {
    switch (stat) {
        case UpgradeStat::Damage:
            return config_.towerUpgrades.damage;
        case UpgradeStat::AttackSpeed:
            return config_.towerUpgrades.attackSpeed;
        case UpgradeStat::Range:
            return config_.towerUpgrades.range;
    }
    return config_.towerUpgrades.damage;
}

int TowerUpgrades::levelFor(UpgradeStat stat) const {
    switch (stat) {
        case UpgradeStat::Damage:
            return damageLevel_;
        case UpgradeStat::AttackSpeed:
            return attackSpeedLevel_;
        case UpgradeStat::Range:
            return rangeLevel_;
    }
    return 0;
}

void TowerUpgrades::setLevelFor(UpgradeStat stat, int level) {
    switch (stat) {
        case UpgradeStat::Damage:
            damageLevel_ = level;
            break;
        case UpgradeStat::AttackSpeed:
            attackSpeedLevel_ = level;
            break;
        case UpgradeStat::Range:
            rangeLevel_ = level;
            break;
    }
}

const char* TowerUpgrades::nameFor(UpgradeStat stat) {
    switch (stat) {
        case UpgradeStat::Damage:
            return "Damage";
        case UpgradeStat::AttackSpeed:
            return "Attack Speed";
        case UpgradeStat::Range:
            return "Range";
    }
    return "";
}

int TowerUpgrades::level(UpgradeStat stat) const {
    return levelFor(stat);
}

int TowerUpgrades::nextCost(UpgradeStat stat) const {
    return upgradeCost(upgradeConfigFor(stat), levelFor(stat));
}

bool TowerUpgrades::purchase(UpgradeStat stat, GameState& state) {
    const int cost = nextCost(stat);
    if (!state.canAfford(cost)) {
        setNotification(std::string(nameFor(stat)) + " upgrade needs " + std::to_string(cost) + " " +
                         config_.economy.currencyName);
        return false;
    }

    state.spend(cost);
    setLevelFor(stat, levelFor(stat) + 1);
    setNotification(std::string(nameFor(stat)) + " upgraded to level " + std::to_string(levelFor(stat)));
    return true;
}

float TowerUpgrades::effectiveDamage() const {
    return upgradedStatValue(config_.tower.damage, config_.towerUpgrades.damage, damageLevel_);
}

float TowerUpgrades::effectiveAttacksPerSecond() const {
    return upgradedStatValue(config_.tower.attacksPerSecond, config_.towerUpgrades.attackSpeed, attackSpeedLevel_);
}

float TowerUpgrades::effectiveRangeTiles() const {
    return upgradedStatValue(config_.tower.rangeTiles, config_.towerUpgrades.range, rangeLevel_);
}

void TowerUpgrades::update(float dtSeconds) {
    if (notificationTimer_ > 0.0f) {
        notificationTimer_ -= dtSeconds;
    }
}

void TowerUpgrades::setNotification(const std::string& message) {
    notification_ = message;
    notificationTimer_ = kNotificationSeconds;
}

} // namespace td
