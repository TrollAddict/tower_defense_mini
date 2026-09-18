#pragma once

#include <string>
#include <vector>

#include <SFML/Graphics.hpp>
#include <entt/entt.hpp>

#include "core/Grid.hpp"
#include "game/CombatSystem.hpp"
#include "game/GameState.hpp"
#include "game/TowerUpgrades.hpp"
#include "game/WaveDirector.hpp"

namespace td {

// Pixel-art for the world layer, loaded once at startup.
//
// tower/wall are single-tile PNGs (one tile wide/tall, scaled to whatever tileSizePx is
// in use). `enemy` is `TX Player.png`; only its first 32x64 frame is drawn.
//
// The terrain comes from `TX Tileset Grass.png`: the top half is
// 32px grass variants (floor), and the stone slabs in the bottom-left are 16px pieces
// that make up the path -- each path cell is a 2x2 of randomly chosen slabs, which keeps
// the stone at the same pixel density as the grass. Variant choice is a hash of the
// cell, so it's random-looking but stable from frame to frame.
struct WorldTextures {
    sf::Texture tileset;
    sf::Texture tower;
    sf::Texture wall;
    sf::Texture enemy;

    std::vector<sf::IntRect> grassTiles;
    std::vector<sf::IntRect> stoneTiles;

    // Whole-map floor, built once by buildFloor() and drawn as a single call.
    sf::VertexBuffer floorBuffer{sf::PrimitiveType::Triangles, sf::VertexBuffer::Usage::Static};

    // Loads the tileset + sprites from `assetsDir`; false if any is missing/unusable.
    bool load(const std::string& assetsDir);

    // Fills floorBuffer with a random grass tile per grid cell. False if the GPU can't
    // do vertex buffers.
    bool buildFloor(const Grid& grid, float tileSizePx);
};

// Draws the world in grid/camera space: spawn + castle markers, towers, walls,
// enemies (with a health bar), and this frame's tower-fire tracer lines. Everything
// here reads GDD §12's "no grid-overlay/gap-width indicators needed" call -- there's
// only one gap-size question ("does a path exist"), and §7 answers it automatically.
//
// `path` is the route enemies currently take from spawn to castle; it's paved with stone.
void renderWorld(sf::RenderWindow& window, const Grid& grid, entt::registry& registry, const ShotList& shots,
                  const std::vector<CellCoord>& path, const WorldTextures& textures, float tileSizePx);

// HUD text drawn in screen space, independent of the camera view (GDD §12).
struct HudInfo {
    const GameConfig* config = nullptr;
    const GameState* state = nullptr;
    const WaveDirector* waveDirector = nullptr;
    const TowerUpgrades* upgrades = nullptr;
    std::string notification;
};

void renderHud(sf::RenderWindow& window, sf::Font& font, const HudInfo& info);

} // namespace td
