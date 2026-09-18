#pragma once

#include <string>

#include <SFML/Graphics.hpp>
#include <entt/entt.hpp>

#include "core/Grid.hpp"
#include "game/CombatSystem.hpp"
#include "game/GameState.hpp"
#include "game/TowerUpgrades.hpp"
#include "game/WaveDirector.hpp"

namespace td {

// Draws the world in grid/camera space: spawn + castle markers, towers, walls,
// enemies (with a health bar), and this frame's tower-fire tracer lines. Everything
// here reads GDD §12's "no grid-overlay/gap-width indicators needed" call -- there's
// only one gap-size question ("does a path exist"), and §7 answers it automatically.
void renderWorld(sf::RenderWindow& window, const Grid& grid, entt::registry& registry, const ShotList& shots,
                  float tileSizePx);

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
