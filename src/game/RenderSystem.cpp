#include "game/RenderSystem.hpp"

#include <algorithm>
#include <sstream>

#include "ecs/Components.hpp"

namespace td {

namespace {

sf::Color toSfColor(const TintColor& c) {
    return sf::Color(c.r, c.g, c.b);
}

void drawCoarseGrid(sf::RenderWindow& window, const Grid& grid, float tileSizePx) {
    constexpr int kLineSpacingCells = 16;
    const float worldW = grid.width() * tileSizePx;
    const float worldH = grid.height() * tileSizePx;
    const sf::Color lineColor(70, 70, 80, 120);

    sf::VertexArray lines(sf::PrimitiveType::Lines);
    for (int x = 0; x <= grid.width(); x += kLineSpacingCells) {
        const float px = x * tileSizePx;
        lines.append(sf::Vertex{sf::Vector2f(px, 0.0f), lineColor});
        lines.append(sf::Vertex{sf::Vector2f(px, worldH), lineColor});
    }
    for (int y = 0; y <= grid.height(); y += kLineSpacingCells) {
        const float py = y * tileSizePx;
        lines.append(sf::Vertex{sf::Vector2f(0.0f, py), lineColor});
        lines.append(sf::Vertex{sf::Vector2f(worldW, py), lineColor});
    }
    window.draw(lines);
}

void drawMarker(sf::RenderWindow& window, const CellCoord& cell, float tileSizePx, sf::Color color) {
    sf::RectangleShape shape(sf::Vector2f(tileSizePx, tileSizePx));
    shape.setPosition(sf::Vector2f(cell.x * tileSizePx, cell.y * tileSizePx));
    shape.setFillColor(sf::Color::Transparent);
    shape.setOutlineColor(color);
    shape.setOutlineThickness(2.0f);
    window.draw(shape);
}

} // namespace

void renderWorld(sf::RenderWindow& window, const Grid& grid, entt::registry& registry, const ShotList& shots,
                  float tileSizePx) {
    const float worldW = grid.width() * tileSizePx;
    const float worldH = grid.height() * tileSizePx;

    sf::RectangleShape background(sf::Vector2f(worldW, worldH));
    background.setPosition(sf::Vector2f(0.0f, 0.0f));
    background.setFillColor(sf::Color(24, 26, 32));
    window.draw(background);

    drawCoarseGrid(window, grid, tileSizePx);
    drawMarker(window, grid.spawnCell(), tileSizePx, sf::Color(80, 220, 120));
    drawMarker(window, grid.castleCell(), tileSizePx, sf::Color(230, 190, 60));

    // Walls.
    {
        auto view = registry.view<WallTag, Position, TintColor>();
        for (auto entity : view) {
            const auto& pos = view.get<Position>(entity);
            const auto& tint = view.get<TintColor>(entity);
            sf::RectangleShape shape(sf::Vector2f(tileSizePx * 0.9f, tileSizePx * 0.9f));
            shape.setOrigin(sf::Vector2f(shape.getSize().x / 2.0f, shape.getSize().y / 2.0f));
            shape.setPosition(sf::Vector2f(pos.x * tileSizePx, pos.y * tileSizePx));
            shape.setFillColor(toSfColor(tint));
            window.draw(shape);
        }
    }

    // Towers.
    {
        auto view = registry.view<TowerState, Position, TintColor>();
        for (auto entity : view) {
            const auto& pos = view.get<Position>(entity);
            const auto& tint = view.get<TintColor>(entity);
            sf::RectangleShape shape(sf::Vector2f(tileSizePx * 0.9f, tileSizePx * 0.9f));
            shape.setOrigin(sf::Vector2f(shape.getSize().x / 2.0f, shape.getSize().y / 2.0f));
            shape.setPosition(sf::Vector2f(pos.x * tileSizePx, pos.y * tileSizePx));
            shape.setFillColor(toSfColor(tint));
            window.draw(shape);
        }
    }

    // Shot tracers (transient, this frame only).
    if (!shots.empty()) {
        sf::VertexArray lines(sf::PrimitiveType::Lines);
        const sf::Color tracerColor(255, 235, 150, 200);
        for (const auto& [from, to] : shots) {
            lines.append(sf::Vertex{sf::Vector2f(from.x * tileSizePx, from.y * tileSizePx), tracerColor});
            lines.append(sf::Vertex{sf::Vector2f(to.x * tileSizePx, to.y * tileSizePx), tracerColor});
        }
        window.draw(lines);
    }

    // Enemies + health bars.
    {
        auto view = registry.view<EnemyTag, Position, Health, TintColor>();
        for (auto entity : view) {
            const auto& pos = view.get<Position>(entity);
            const auto& health = view.get<Health>(entity);
            const auto& tint = view.get<TintColor>(entity);

            const float radius = tileSizePx * 0.3f;
            sf::CircleShape shape(radius);
            shape.setOrigin(sf::Vector2f(radius, radius));
            shape.setPosition(sf::Vector2f(pos.x * tileSizePx, pos.y * tileSizePx));
            shape.setFillColor(toSfColor(tint));
            window.draw(shape);

            const float barWidth = tileSizePx * 0.7f;
            const float healthFrac = health.max > 0.0f ? std::max(0.0f, health.current / health.max) : 0.0f;

            sf::RectangleShape barBack(sf::Vector2f(barWidth, 3.0f));
            barBack.setOrigin(sf::Vector2f(barWidth / 2.0f, 0.0f));
            barBack.setPosition(sf::Vector2f(pos.x * tileSizePx, pos.y * tileSizePx - radius - 6.0f));
            barBack.setFillColor(sf::Color(60, 60, 60));
            window.draw(barBack);

            sf::RectangleShape barFront(sf::Vector2f(barWidth * healthFrac, 3.0f));
            barFront.setOrigin(sf::Vector2f(0.0f, 0.0f));
            barFront.setPosition(sf::Vector2f(pos.x * tileSizePx - barWidth / 2.0f, pos.y * tileSizePx - radius - 6.0f));
            barFront.setFillColor(sf::Color(90, 220, 90));
            window.draw(barFront);
        }
    }
}

void renderHud(sf::RenderWindow& window, sf::Font& font, const HudInfo& info) {
    std::ostringstream oss;
    oss << info.config->economy.currencyName << ": " << info.state->currency() << "   "
        << "Castle HP: " << static_cast<int>(info.state->castleHealth()) << "/"
        << static_cast<int>(info.state->castleMaxHealth()) << "   "
        << "Wave: " << info.state->waveNumber() << "   "
        << "Kills: " << info.state->kills();

    if (info.waveDirector != nullptr) {
        if (info.waveDirector->enemiesRemainingToSpawn() > 0) {
            oss << "   Spawning: " << info.waveDirector->enemiesRemainingToSpawn() << " left";
        } else if (info.waveDirector->intermissionSecondsRemaining() > 0.0f) {
            oss << "   Next wave in: " << static_cast<int>(info.waveDirector->intermissionSecondsRemaining()) + 1
                << "s (ENTER to skip)";
        }
    }

    if (info.upgrades != nullptr) {
        oss << "\nTower -- Dmg Lv" << info.upgrades->level(UpgradeStat::Damage) << " ("
            << info.upgrades->nextCost(UpgradeStat::Damage) << ")"
            << "   AtkSpd Lv" << info.upgrades->level(UpgradeStat::AttackSpeed) << " ("
            << info.upgrades->nextCost(UpgradeStat::AttackSpeed) << ")"
            << "   Range Lv" << info.upgrades->level(UpgradeStat::Range) << " ("
            << info.upgrades->nextCost(UpgradeStat::Range) << ")";
    }

    sf::Text hudText(font, oss.str(), 16);
    hudText.setPosition(sf::Vector2f(10.0f, 10.0f));
    hudText.setFillColor(sf::Color::White);
    window.draw(hudText);

    sf::Text helpText(font,
                       "LMB: place selected  RMB: remove  1: Tower  2: Wall  3/4/5: upgrade Dmg/AtkSpd/Range\n"
                       "ENTER: skip intermission  G: give up on wave (stragglers hit the castle)  ESC: menu",
                       13);
    helpText.setPosition(sf::Vector2f(10.0f, 54.0f));
    helpText.setFillColor(sf::Color(180, 180, 180));
    window.draw(helpText);

    if (!info.notification.empty()) {
        sf::Text note(font, info.notification, 18);
        note.setFillColor(sf::Color(255, 120, 120));
        const auto bounds = note.getLocalBounds();
        note.setPosition(sf::Vector2f((window.getSize().x - bounds.size.x) / 2.0f, 98.0f));
        window.draw(note);
    }
}

} // namespace td
