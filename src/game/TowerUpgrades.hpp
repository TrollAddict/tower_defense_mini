#pragma once

#include <string>

#include "core/Config.hpp"
#include "game/GameState.hpp"

namespace td {

enum class UpgradeStat { Damage, AttackSpeed, Range };

// Global, permanent stat upgrades for the single tower type. The original GDD scope
// explicitly had no upgrades/tech tree (§6/§9); this is a deliberate later addition
// -- see GDD §6's updated note. A purchase costs currency and applies immediately to
// every tower on the field, already-placed or future: there's no per-tower level to
// track, which is what keeps this in scope for a one-tower-type game -- no tower
// selection UI, no per-entity upgrade bookkeeping, just three global counters.
class TowerUpgrades {
public:
    explicit TowerUpgrades(const GameConfig& config);

    // Attempts to buy the next level of `stat` from `state`'s currency. Returns true
    // if purchased; sets a short-lived HUD notification either way.
    bool purchase(UpgradeStat stat, GameState& state);

    int level(UpgradeStat stat) const;
    int nextCost(UpgradeStat stat) const;

    float effectiveDamage() const;
    float effectiveAttacksPerSecond() const;
    float effectiveRangeTiles() const;

    void update(float dtSeconds);
    const std::string& notification() const { return notification_; }
    bool hasActiveNotification() const { return notificationTimer_ > 0.0f; }

private:
    const GameConfig& config_;
    int damageLevel_ = 0;
    int attackSpeedLevel_ = 0;
    int rangeLevel_ = 0;
    std::string notification_;
    float notificationTimer_ = 0.0f;

    const StatUpgradeConfig& upgradeConfigFor(UpgradeStat stat) const;
    int levelFor(UpgradeStat stat) const;
    void setLevelFor(UpgradeStat stat, int level);
    static const char* nameFor(UpgradeStat stat);
    void setNotification(const std::string& message);
};

} // namespace td
