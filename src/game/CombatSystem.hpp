#pragma once

#include <utility>
#include <vector>

#include <entt/entt.hpp>

#include "core/Config.hpp"
#include "ecs/Components.hpp"
#include "game/GameState.hpp"

namespace td {

// One shot fired this frame, tower position to target position (both grid-space),
// for RenderSystem to draw a brief tracer line. Purely visual, not simulation state.
using ShotList = std::vector<std::pair<Position, Position>>;

// The single tower type (GDD §6) targets the nearest enemy in range and fires at its
// configured rate. No upgrade tiers, no splash, no ammo -- there's nothing else for
// combat to do at this scope.
void updateCombat(entt::registry& registry, GameState& state, const GameConfig& config, float dtSeconds,
                   ShotList& outShots);

} // namespace td
