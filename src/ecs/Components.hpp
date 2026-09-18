#pragma once

#include <cstdint>

namespace td {

// Grid-space position in tile units (float so movement between cells is smooth).
struct Position {
    float x = 0.0f;
    float y = 0.0f;
};

struct Health {
    float current = 0.0f;
    float max = 0.0f;
};

// Per-entity so each enemy keeps the speed it spawned with (GDD §5 escalation),
// same reasoning as Health being per-entity rather than read from config directly.
struct Speed {
    float tilesPerSecond = 0.0f;
};

// Deliberately empty — GDD §5, only one enemy archetype, no per-enemy variant data.
struct EnemyTag {};

// Deliberately empty — GDD §6, walls have no HP/behavior; they exist purely as an
// occupied grid cell rendered from this tag entity.
struct WallTag {};

struct TowerState {
    int gridX = 0;
    int gridY = 0;
    float cooldownRemaining = 0.0f;
};

// Kept SFML-free so core ECS state doesn't depend on the render backend; RenderSystem
// maps this to sf::Color.
struct TintColor {
    std::uint8_t r = 255;
    std::uint8_t g = 255;
    std::uint8_t b = 255;
};

} // namespace td
