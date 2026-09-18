#include "game/PlacementController.hpp"

#include "ecs/Components.hpp"

namespace td {

namespace {
constexpr float kNotificationSeconds = 2.5f;

constexpr TintColor kTowerColor{60, 130, 240};
constexpr TintColor kWallColor{140, 140, 140};
} // namespace

PlacementController::PlacementController(Grid& grid, FlowField& flowField, entt::registry& registry,
                                           GameState& state, const GameConfig& config)
    : grid_(grid),
      flowField_(flowField),
      registry_(registry),
      state_(state),
      config_(config),
      entityAt_(static_cast<std::size_t>(grid.width()) * static_cast<std::size_t>(grid.height()),
                entt::entity{entt::null}) {}

void PlacementController::setNotification(const std::string& message) {
    notification_ = message;
    notificationTimer_ = kNotificationSeconds;
}

bool PlacementController::place(StructureType type, int x, int y) {
    if (!grid_.isBuildable(x, y)) {
        setNotification("Can't build there");
        return false;
    }

    const int cost = (type == StructureType::Tower) ? config_.tower.cost : config_.wall.cost;
    if (!state_.canAfford(cost)) {
        setNotification("Not enough " + config_.economy.currencyName);
        return false;
    }

    state_.spend(cost);
    const CellType cellType = (type == StructureType::Tower) ? CellType::Tower : CellType::Wall;
    grid_.setCell(x, y, cellType);
    flowField_.compute(grid_);

    const CellCoord& spawn = grid_.spawnCell();
    if (!flowField_.isReachable(spawn.x, spawn.y)) {
        // Revert: this is the "player's last tower or wall is removed" rule from the
        // brief. Only the edit just made can ever be the cause, since every prior
        // edit already passed this same check.
        grid_.clearCell(x, y);
        state_.refund(cost);
        flowField_.compute(grid_);
        setNotification("No path -- placement reverted");
        return false;
    }

    entt::entity entity = registry_.create();
    registry_.emplace<Position>(entity, x + 0.5f, y + 0.5f);
    if (type == StructureType::Tower) {
        registry_.emplace<TowerState>(entity, x, y, 0.0f);
        registry_.emplace<TintColor>(entity, kTowerColor);
    } else {
        registry_.emplace<WallTag>(entity);
        registry_.emplace<TintColor>(entity, kWallColor);
    }
    entityAt_[index(x, y)] = entity;

    return true;
}

void PlacementController::remove(int x, int y) {
    if (!grid_.inBounds(x, y)) return;
    if (grid_.at(x, y) == CellType::Empty) return;

    entt::entity entity = entityAt_[index(x, y)];
    if (registry_.valid(entity)) {
        registry_.destroy(entity);
    }
    entityAt_[index(x, y)] = entt::null;

    grid_.clearCell(x, y);
    flowField_.compute(grid_);
}

void PlacementController::update(float dtSeconds) {
    if (notificationTimer_ > 0.0f) {
        notificationTimer_ -= dtSeconds;
    }
}

} // namespace td
