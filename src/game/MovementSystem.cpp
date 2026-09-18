#include "game/MovementSystem.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

#include "core/Movement.hpp"
#include "ecs/Components.hpp"

namespace td {

namespace {
constexpr float kArrivalRadiusTiles = 0.35f;
}

void updateMovement(entt::registry& registry, const Grid& grid, const FlowField& flowField, GameState& state,
                     const GameConfig& config, float dtSeconds) {
    const CellCoord& castle = grid.castleCell();
    const float castleCenterX = castle.x + 0.5f;
    const float castleCenterY = castle.y + 0.5f;

    std::vector<entt::entity> arrived;

    auto view = registry.view<Position, Speed, EnemyTag>();
    for (auto entity : view) {
        auto& pos = view.get<Position>(entity);
        const float speed = view.get<Speed>(entity).tilesPerSecond;

        const float dx = castleCenterX - pos.x;
        const float dy = castleCenterY - pos.y;
        if (std::sqrt(dx * dx + dy * dy) <= kArrivalRadiusTiles) {
            arrived.push_back(entity);
            continue;
        }

        const int cellX = std::clamp(static_cast<int>(std::floor(pos.x)), 0, grid.width() - 1);
        const int cellY = std::clamp(static_cast<int>(std::floor(pos.y)), 0, grid.height() - 1);
        const Vec2 dir = flowField.direction(cellX, cellY);

        float newX = pos.x + dir.x * speed * dtSeconds;
        float newY = pos.y + dir.y * speed * dtSeconds;
        resolveCornerClipping(grid, cellX, cellY, newX, newY);
        pos.x = newX;
        pos.y = newY;
    }

    for (auto entity : arrived) {
        state.damageCastle(config.enemy.castleDamage);
        registry.destroy(entity);
    }
}

} // namespace td
