#pragma once

#include <optional>

#include <SFML/Graphics.hpp>
#include <entt/entt.hpp>

#include "core/Config.hpp"
#include "core/FlowField.hpp"
#include "core/Grid.hpp"
#include "game/CombatSystem.hpp"
#include "game/GameState.hpp"
#include "game/PlacementController.hpp"
#include "game/WaveDirector.hpp"

namespace td {

enum class AppState { MainMenu, Playing, GameOver };

// Top-level orchestration: main menu (difficulty sliders, GDD §14) -> playing (the
// actual GDD §3 loop) -> game over (castle destroyed, GDD §3's lose condition) -> back
// to menu. One run's ECS/grid/flow-field state is torn down and recreated on Start,
// same map layout every time (GDD §4 -- single hand-authored map, no regeneration).
class Game {
public:
    Game();
    void run();

private:
    sf::RenderWindow window_;
    sf::Font font_;
    GameConfig config_;
    AppState state_ = AppState::MainMenu;

    // Menu-selected difficulty, applied to the next run started (GDD §14).
    DifficultyConfig menuDifficulty_;

    // Live-run state; only populated while state_ == Playing/GameOver.
    std::optional<Grid> grid_;
    FlowField flowField_;
    entt::registry registry_;
    std::optional<GameState> gameState_;
    std::optional<PlacementController> placement_;
    std::optional<WaveDirector> waveDirector_;
    StructureType selectedStructure_ = StructureType::Tower;
    ShotList frameShots_;

    float tileSizePx_ = 32.0f; // GDD §4 default tile size
    sf::Vector2f cameraCenter_{0.0f, 0.0f};
    float zoom_ = 1.0f;

    void handleEvents();

    void updateMenu();
    void renderMenu();

    void startNewRun();
    void updatePlaying(float dtSeconds);
    void renderPlaying();
    void handlePlayingClick(sf::Vector2i pixel, bool isLeftButton);
    void handlePlayingScroll(float delta);
    void panCamera(float dtSeconds);
    float fitZoom() const;
    CellCoord pixelToCell(sf::Vector2i pixel) const;

    void renderGameOver();
};

} // namespace td
