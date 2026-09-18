#include "game/CombatSystem.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace td {

void updateCombat(entt::registry& registry, GameState& state, const GameConfig& config, float dtSeconds,
                   ShotList& outShots) {
    const float range = config.tower.rangeTiles;
    const float rangeSq = range * range;
    const float cooldownDuration = 1.0f / std::max(0.01f, config.tower.attacksPerSecond);

    auto towers = registry.view<TowerState, Position>();
    auto enemies = registry.view<Position, Health, EnemyTag>();

    for (auto towerEntity : towers) {
        auto& tower = towers.get<TowerState>(towerEntity);
        const auto& towerPos = towers.get<Position>(towerEntity);

        tower.cooldownRemaining = std::max(0.0f, tower.cooldownRemaining - dtSeconds);
        if (tower.cooldownRemaining > 0.0f) continue;

        entt::entity best = entt::null;
        float bestDistSq = std::numeric_limits<float>::max();
        for (auto enemyEntity : enemies) {
            const auto& enemyPos = enemies.get<Position>(enemyEntity);
            const float dx = enemyPos.x - towerPos.x;
            const float dy = enemyPos.y - towerPos.y;
            const float distSq = dx * dx + dy * dy;
            if (distSq <= rangeSq && distSq < bestDistSq) {
                bestDistSq = distSq;
                best = enemyEntity;
            }
        }

        if (best == entt::null) continue;

        tower.cooldownRemaining = cooldownDuration;
        auto& health = enemies.get<Health>(best);
        health.current -= config.tower.damage;
        outShots.emplace_back(towerPos, enemies.get<Position>(best));

        if (health.current <= 0.0f) {
            state.addCurrency(config.enemy.killReward);
            state.addKill();
            registry.destroy(best);
        }
    }
}

} // namespace td
