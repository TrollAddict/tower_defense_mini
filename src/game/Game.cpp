#include "game/Game.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>

#include "game/MovementSystem.hpp"
#include "game/RenderSystem.hpp"

namespace td {

namespace {

constexpr float kSliderX = 440.0f;
constexpr float kSliderWidth = 400.0f;
constexpr float kSliderHeight = 20.0f;
constexpr float kSlider1Y = 260.0f;
constexpr float kSlider2Y = 330.0f;
constexpr float kSlider3Y = 400.0f;
constexpr float kStartButtonX = 540.0f;
constexpr float kStartButtonY = 480.0f;
constexpr float kStartButtonW = 200.0f;
constexpr float kStartButtonH = 50.0f;

sf::FloatRect sliderRect(float y) {
    return sf::FloatRect({kSliderX, y}, {kSliderWidth, kSliderHeight});
}

sf::FloatRect startButtonRect() {
    return sf::FloatRect({kStartButtonX, kStartButtonY}, {kStartButtonW, kStartButtonH});
}

} // namespace

Game::Game() : window_(sf::VideoMode({1280u, 720u}), "Tower Defense Mini"), config_(loadGameConfig(TD_MINI_DATA_DIR)) {
    window_.setFramerateLimit(60);
    if (!font_.openFromFile(std::string(TD_MINI_ASSETS_DIR) + "/DejaVuSans.ttf")) {
        throw std::runtime_error("Failed to load font from " TD_MINI_ASSETS_DIR "/DejaVuSans.ttf");
    }
    menuDifficulty_ = config_.difficulty;
}

void Game::run() {
    sf::Clock clock;
    while (window_.isOpen()) {
        const float dt = clock.restart().asSeconds();
        handleEvents();

        if (state_ == AppState::MainMenu) {
            updateMenu();
        } else if (state_ == AppState::Playing) {
            updatePlaying(dt);
        }

        window_.clear(sf::Color(15, 15, 20));
        if (state_ == AppState::MainMenu) {
            renderMenu();
        } else if (state_ == AppState::Playing) {
            renderPlaying();
        } else {
            renderGameOver();
        }
        window_.display();
    }
}

void Game::handleEvents() {
    while (const std::optional event = window_.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window_.close();
        } else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
            if (state_ == AppState::Playing) {
                if (keyPressed->code == sf::Keyboard::Key::Num1) {
                    selectedStructure_ = StructureType::Tower;
                } else if (keyPressed->code == sf::Keyboard::Key::Num2) {
                    selectedStructure_ = StructureType::Wall;
                } else if (keyPressed->code == sf::Keyboard::Key::Escape) {
                    state_ = AppState::MainMenu;
                } else if (keyPressed->code == sf::Keyboard::Key::Home) {
                    zoom_ = fitZoom();
                } else if (keyPressed->code == sf::Keyboard::Key::Enter) {
                    waveDirector_->skipIntermission();
                } else if (keyPressed->code == sf::Keyboard::Key::Num3) {
                    towerUpgrades_->purchase(UpgradeStat::Damage, *gameState_);
                } else if (keyPressed->code == sf::Keyboard::Key::Num4) {
                    towerUpgrades_->purchase(UpgradeStat::AttackSpeed, *gameState_);
                } else if (keyPressed->code == sf::Keyboard::Key::Num5) {
                    towerUpgrades_->purchase(UpgradeStat::Range, *gameState_);
                }
            } else if (state_ == AppState::GameOver) {
                if (keyPressed->code == sf::Keyboard::Key::Enter) {
                    state_ = AppState::MainMenu;
                }
            }
        } else if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>()) {
            const sf::Vector2f mouse(static_cast<float>(mousePressed->position.x),
                                      static_cast<float>(mousePressed->position.y));
            if (state_ == AppState::Playing) {
                if (mousePressed->button == sf::Mouse::Button::Left) {
                    handlePlayingClick(mousePressed->position, true);
                } else if (mousePressed->button == sf::Mouse::Button::Right) {
                    handlePlayingClick(mousePressed->position, false);
                }
            } else if (state_ == AppState::MainMenu) {
                if (mousePressed->button == sf::Mouse::Button::Left && startButtonRect().contains(mouse)) {
                    startNewRun();
                }
            }
        } else if (const auto* scrolled = event->getIf<sf::Event::MouseWheelScrolled>()) {
            if (state_ == AppState::Playing) {
                handlePlayingScroll(scrolled->delta);
            }
        }
    }
}

void Game::updateMenu() {
    if (!sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) return;

    const sf::Vector2i mousePixel = sf::Mouse::getPosition(window_);
    const sf::Vector2f mouse(static_cast<float>(mousePixel.x), static_cast<float>(mousePixel.y));

    auto applySlider = [&](const sf::FloatRect& rect, int& value) {
        if (!rect.contains(mouse)) return;
        const float t = std::clamp((mouse.x - rect.position.x) / rect.size.x, 0.0f, 1.0f);
        value = config_.difficulty.minValue +
                static_cast<int>(std::round(t * static_cast<float>(config_.difficulty.maxValue - config_.difficulty.minValue)));
    };

    applySlider(sliderRect(kSlider1Y), menuDifficulty_.waveSizeMultiplier);
    applySlider(sliderRect(kSlider2Y), menuDifficulty_.spawnIntervalMultiplier);
    applySlider(sliderRect(kSlider3Y), menuDifficulty_.enemyHealthMultiplier);
}

void Game::renderMenu() {
    window_.setView(window_.getDefaultView());

    sf::Text title(font_, "Tower Defense Mini", 36);
    title.setPosition({440.0f, 120.0f});
    title.setFillColor(sf::Color::White);
    window_.draw(title);

    sf::Text subtitle(font_, "256x256 grid  --  1 tower  --  1 wall  --  1 enemy", 16);
    subtitle.setPosition({440.0f, 175.0f});
    subtitle.setFillColor(sf::Color(170, 170, 170));
    window_.draw(subtitle);

    auto drawSlider = [&](const std::string& label, float y, int value) {
        sf::Text labelText(font_, label + ": " + std::to_string(value), 16);
        labelText.setPosition({kSliderX, y - 22.0f});
        labelText.setFillColor(sf::Color::White);
        window_.draw(labelText);

        sf::RectangleShape track(sf::Vector2f(kSliderWidth, kSliderHeight));
        track.setPosition({kSliderX, y});
        track.setFillColor(sf::Color(60, 60, 70));
        window_.draw(track);

        const float t = static_cast<float>(value - config_.difficulty.minValue) /
                         static_cast<float>(config_.difficulty.maxValue - config_.difficulty.minValue);
        sf::RectangleShape handle(sf::Vector2f(10.0f, kSliderHeight + 8.0f));
        handle.setPosition({kSliderX + t * kSliderWidth - 5.0f, y - 4.0f});
        handle.setFillColor(sf::Color(230, 190, 60));
        window_.draw(handle);
    };

    drawSlider("Wave size multiplier", kSlider1Y, menuDifficulty_.waveSizeMultiplier);
    drawSlider("Spawn interval multiplier", kSlider2Y, menuDifficulty_.spawnIntervalMultiplier);
    drawSlider("Enemy health multiplier", kSlider3Y, menuDifficulty_.enemyHealthMultiplier);

    sf::RectangleShape startButton(sf::Vector2f(kStartButtonW, kStartButtonH));
    startButton.setPosition({kStartButtonX, kStartButtonY});
    startButton.setFillColor(sf::Color(70, 160, 90));
    window_.draw(startButton);

    sf::Text startText(font_, "Start", 22);
    const auto bounds = startText.getLocalBounds();
    startText.setPosition({kStartButtonX + (kStartButtonW - bounds.size.x) / 2.0f - bounds.position.x,
                            kStartButtonY + (kStartButtonH - bounds.size.y) / 2.0f - bounds.position.y});
    startText.setFillColor(sf::Color::White);
    window_.draw(startText);
}

void Game::startNewRun() {
    registry_.clear();
    grid_.emplace(256, 256, CellCoord{0, 0}, CellCoord{255, 255});
    flowField_.compute(*grid_);
    gameState_.emplace(config_);
    gameState_->difficulty() = menuDifficulty_;
    placement_.emplace(*grid_, flowField_, registry_, *gameState_, config_);
    waveDirector_.emplace(*grid_, registry_, *gameState_, config_);
    towerUpgrades_.emplace(config_);
    selectedStructure_ = StructureType::Tower;

    // Start zoomed in enough to actually see towers/enemies (tile == 1 screen px at
    // zoom 1, since tileSizePx_ is already in world-space pixels) and centered on the
    // spawn point, where the action starts. fitZoom() (bound to Home) is for the
    // on-demand whole-map overview GDD §12 asks for -- it was never meant to be the
    // default view, since at 256 tiles across it shrinks every entity to sub-pixel.
    const CellCoord& spawn = grid_->spawnCell();
    cameraCenter_ = sf::Vector2f((spawn.x + 0.5f) * tileSizePx_, (spawn.y + 0.5f) * tileSizePx_);
    zoom_ = 1.0f;

    state_ = AppState::Playing;
}

void Game::updatePlaying(float dtSeconds) {
    panCamera(dtSeconds);

    frameShots_.clear();
    updateCombat(registry_, *gameState_, config_, *towerUpgrades_, dtSeconds, frameShots_);
    updateMovement(registry_, *grid_, flowField_, *gameState_, config_, dtSeconds);
    waveDirector_->update(dtSeconds);
    placement_->update(dtSeconds);
    towerUpgrades_->update(dtSeconds);

    if (gameState_->isCastleDestroyed()) {
        state_ = AppState::GameOver;
    }
}

void Game::panCamera(float dtSeconds) {
    const float speed = 700.0f * zoom_;
    sf::Vector2f delta{0.0f, 0.0f};
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)) {
        delta.y -= 1.0f;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) {
        delta.y += 1.0f;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) {
        delta.x -= 1.0f;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
        delta.x += 1.0f;
    }

    cameraCenter_.x += delta.x * speed * dtSeconds;
    cameraCenter_.y += delta.y * speed * dtSeconds;

    const float worldW = grid_->width() * tileSizePx_;
    const float worldH = grid_->height() * tileSizePx_;
    const float viewW = window_.getSize().x * zoom_;
    const float viewH = window_.getSize().y * zoom_;

    if (viewW >= worldW) {
        cameraCenter_.x = worldW / 2.0f;
    } else {
        cameraCenter_.x = std::clamp(cameraCenter_.x, viewW / 2.0f, worldW - viewW / 2.0f);
    }

    if (viewH >= worldH) {
        cameraCenter_.y = worldH / 2.0f;
    } else {
        cameraCenter_.y = std::clamp(cameraCenter_.y, viewH / 2.0f, worldH - viewH / 2.0f);
    }
}

float Game::fitZoom() const {
    const float worldW = grid_->width() * tileSizePx_;
    const float worldH = grid_->height() * tileSizePx_;
    return std::max(worldW / static_cast<float>(window_.getSize().x), worldH / static_cast<float>(window_.getSize().y));
}

CellCoord Game::pixelToCell(sf::Vector2i pixel) const {
    const sf::View gameView(cameraCenter_,
                             sf::Vector2f(window_.getSize().x * zoom_, window_.getSize().y * zoom_));
    const sf::Vector2f world = window_.mapPixelToCoords(pixel, gameView);
    return CellCoord{static_cast<int>(std::floor(world.x / tileSizePx_)), static_cast<int>(std::floor(world.y / tileSizePx_))};
}

void Game::handlePlayingClick(sf::Vector2i pixel, bool isLeftButton) {
    if (pixel.y < 100) return; // reserve the top HUD strip (now two HUD lines + help text) from placement clicks

    const CellCoord cell = pixelToCell(pixel);
    if (!grid_->inBounds(cell.x, cell.y)) return;

    if (isLeftButton) {
        placement_->place(selectedStructure_, cell.x, cell.y);
    } else {
        placement_->remove(cell.x, cell.y);
    }
}

void Game::handlePlayingScroll(float delta) {
    const float factor = (delta > 0.0f) ? (1.0f / 1.1f) : 1.1f;
    zoom_ = std::clamp(zoom_ * factor, 0.35f, fitZoom());
}

void Game::renderPlaying() {
    const sf::View gameView(cameraCenter_,
                             sf::Vector2f(window_.getSize().x * zoom_, window_.getSize().y * zoom_));
    window_.setView(gameView);
    renderWorld(window_, *grid_, registry_, frameShots_, tileSizePx_);

    window_.setView(window_.getDefaultView());
    HudInfo hud;
    hud.config = &config_;
    hud.state = &(*gameState_);
    hud.waveDirector = &(*waveDirector_);
    hud.upgrades = &(*towerUpgrades_);
    if (placement_->hasActiveNotification()) {
        hud.notification = placement_->notification();
    } else if (towerUpgrades_->hasActiveNotification()) {
        hud.notification = towerUpgrades_->notification();
    }
    renderHud(window_, font_, hud);
}

void Game::renderGameOver() {
    window_.setView(window_.getDefaultView());

    sf::Text title(font_, "Castle destroyed", 32);
    title.setPosition({470.0f, 260.0f});
    title.setFillColor(sf::Color(230, 90, 90));
    window_.draw(title);

    std::ostringstream oss;
    oss << "Survived to wave " << gameState_->waveNumber() << "  --  " << gameState_->kills() << " kills";
    sf::Text stats(font_, oss.str(), 18);
    stats.setPosition({470.0f, 310.0f});
    stats.setFillColor(sf::Color::White);
    window_.draw(stats);

    sf::Text hint(font_, "Press ENTER to return to the menu", 16);
    hint.setPosition({470.0f, 350.0f});
    hint.setFillColor(sf::Color(180, 180, 180));
    window_.draw(hint);
}

} // namespace td
