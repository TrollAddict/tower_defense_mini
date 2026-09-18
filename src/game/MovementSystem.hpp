#pragma once

#include <entt/entt.hpp>

#include "core/Config.hpp"
#include "core/FlowField.hpp"
#include "core/Grid.hpp"
#include "game/GameState.hpp"

namespace td {

// Moves every enemy along the single shared flow field (GDD §10) and applies castle
// damage + despawn when one arrives (GDD §5's death/cleanup + §3's lose condition).
void updateMovement(entt::registry& registry, const Grid& grid, const FlowField& flowField, GameState& state,
                     const GameConfig& config, float dtSeconds);

} // namespace td
