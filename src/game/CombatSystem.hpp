#pragma once

#include <utility>
#include <vector>

#include <entt/entt.hpp>

#include "core/Config.hpp"
#include "ecs/Components.hpp"
#include "game/GameState.hpp"
#include "game/TowerUpgrades.hpp"

namespace td {

// One shot fired this frame, tower position to target position (both grid-space),
// for RenderSystem to draw a brief tracer line. Purely visual, not simulation state.
using ShotList = std::vector<std::pair<Position, Position>>;

// The single tower type (GDD §6) targets the nearest enemy in range and fires at its
// current rate/damage/range -- `upgrades` supplies those as base-stat-plus-global-
// upgrade-level (see TowerUpgrades), so every tower reflects the latest purchased
// level uniformly. No splash, no ammo -- there's nothing else for combat to do at
// this scope.
void updateCombat(entt::registry& registry, GameState& state, const GameConfig& config, const TowerUpgrades& upgrades,
                   float dtSeconds, ShotList& outShots);

} // namespace td
