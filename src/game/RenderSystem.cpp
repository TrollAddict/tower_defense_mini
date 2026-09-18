#include "game/RenderSystem.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <sstream>

#include "ecs/Components.hpp"

namespace td {

namespace {

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

constexpr unsigned kGrassTilePx = 32;
constexpr unsigned kStonePiecePx = 16;
constexpr int kMinStoneCoveragePercent = 72;

// Enemy art: the first 32x64 frame of `TX Player.png` (one tile wide, two tall). Rows
// are source pixels within that frame: the visible body runs from the top of the head
// to the feet, with transparent padding around it.
const sf::IntRect kEnemyFrame({0, 0}, {32, 64});
constexpr float kEnemyHeadRowPx = 14.0f;
constexpr float kEnemyFeetRowPx = 58.0f;
// Feet stand this far (in tiles) below the entity's position, so the body sits mostly
// above it instead of straddling the cell center.
constexpr float kEnemyFeetOffsetTiles = 0.25f;

// Cheap deterministic per-cell hash (splitmix64 finalizer), so tile variants look random
// but never change between frames or runs. `salt` distinguishes independent picks for
// the same cell.
std::uint64_t cellHash(int x, int y, unsigned salt) {
    std::uint64_t h = (static_cast<std::uint64_t>(static_cast<std::uint32_t>(x)) << 32) ^
                      static_cast<std::uint64_t>(static_cast<std::uint32_t>(y)) ^
                      (static_cast<std::uint64_t>(salt) * 0x9E3779B97F4A7C15ULL);
    h += 0x9E3779B97F4A7C15ULL;
    h = (h ^ (h >> 30)) * 0xBF58476D1CE4E5B9ULL;
    h = (h ^ (h >> 27)) * 0x94D049BB133111EBULL;
    return h ^ (h >> 31);
}

// Appends a `sizePx` square at `origin` textured with `tile` (two triangles).
void appendQuad(std::vector<sf::Vertex>& out, sf::Vector2f origin, float sizePx, const sf::IntRect& tile) {
    const sf::Vector2f tl(origin);
    const sf::Vector2f tr(origin.x + sizePx, origin.y);
    const sf::Vector2f bl(origin.x, origin.y + sizePx);
    const sf::Vector2f br(origin.x + sizePx, origin.y + sizePx);
    const sf::Vector2f uvTl(sf::Vector2f(tile.position));
    const sf::Vector2f uvTr(uvTl.x + static_cast<float>(tile.size.x), uvTl.y);
    const sf::Vector2f uvBl(uvTl.x, uvTl.y + static_cast<float>(tile.size.y));
    const sf::Vector2f uvBr(uvTr.x, uvBl.y);
    out.push_back({tl, sf::Color::White, uvTl});
    out.push_back({tr, sf::Color::White, uvTr});
    out.push_back({bl, sf::Color::White, uvBl});
    out.push_back({bl, sf::Color::White, uvBl});
    out.push_back({tr, sf::Color::White, uvTr});
    out.push_back({br, sf::Color::White, uvBr});
}

// Draws `texture` centered on a tile-space position, scaled so its width spans `sizePx`.
void drawSpriteCentered(sf::RenderWindow& window, const sf::Texture& texture, sf::Vector2f centerPx, float sizePx) {
    sf::Sprite sprite(texture);
    const auto texSize = sf::Vector2f(texture.getSize());
    sprite.setOrigin(texSize / 2.0f);
    sprite.setPosition(centerPx);
    sprite.setScale(sf::Vector2f(sizePx / texSize.x, sizePx / texSize.y));
    window.draw(sprite);
}

} // namespace

bool WorldTextures::load(const std::string& assetsDir) {
    if (!tileset.loadFromFile(assetsDir + "/TX Tileset Grass.png") || !tower.loadFromFile(assetsDir + "/tower.png") ||
        !wall.loadFromFile(assetsDir + "/wall.png") || !enemy.loadFromFile(assetsDir + "/TX Player.png")) {
        return false;
    }

    // Top half of the sheet: 8 x 4 grass tiles.
    const sf::Vector2u size = tileset.getSize();
    for (unsigned y = 0; y < size.y / 2; y += kGrassTilePx) {
        for (unsigned x = 0; x + kGrassTilePx <= size.x; x += kGrassTilePx) {
            grassTiles.push_back(sf::IntRect({static_cast<int>(x), static_cast<int>(y)}, {kGrassTilePx, kGrassTilePx}));
        }
    }

    // Bottom-left quadrant: keep the 16px cells that are mostly stone (skipping the
    // empty gaps between slab groups and slabs that are mostly eaten away by grass).
    const sf::Image image = tileset.copyToImage();
    for (unsigned y = size.y / 2; y + kStonePiecePx <= size.y; y += kStonePiecePx) {
        for (unsigned x = 0; x < size.x / 2; x += kStonePiecePx) {
            int stonePixels = 0;
            for (unsigned py = y; py < y + kStonePiecePx; ++py) {
                for (unsigned px = x; px < x + kStonePiecePx; ++px) {
                    const sf::Color c = image.getPixel({px, py});
                    // Stone is grey (r ~ g ~ b); grass is olive (b far below r/g).
                    if (c.a > 0 && std::abs(c.r - c.g) < 25 && std::abs(c.g - c.b) < 25) ++stonePixels;
                }
            }
            if (stonePixels * 100 >= kStonePiecePx * kStonePiecePx * kMinStoneCoveragePercent) {
                stoneTiles.push_back(sf::IntRect({static_cast<int>(x), static_cast<int>(y)}, {kStonePiecePx, kStonePiecePx}));
            }
        }
    }
    return !grassTiles.empty() && !stoneTiles.empty();
}

bool WorldTextures::buildFloor(const Grid& grid, float tileSizePx) {
    if (!sf::VertexBuffer::isAvailable()) return false;

    std::vector<sf::Vertex> vertices;
    vertices.reserve(static_cast<std::size_t>(grid.width()) * static_cast<std::size_t>(grid.height()) * 6);
    for (int y = 0; y < grid.height(); ++y) {
        for (int x = 0; x < grid.width(); ++x) {
            const sf::IntRect& tile = grassTiles[cellHash(x, y, 0) % grassTiles.size()];
            appendQuad(vertices, sf::Vector2f(x * tileSizePx, y * tileSizePx), tileSizePx, tile);
        }
    }
    return floorBuffer.create(vertices.size()) && floorBuffer.update(vertices.data());
}

void renderWorld(sf::RenderWindow& window, const Grid& grid, entt::registry& registry, const ShotList& shots,
                  const std::vector<CellCoord>& path, const WorldTextures& textures, float tileSizePx) {
    const float worldW = grid.width() * tileSizePx;
    const float worldH = grid.height() * tileSizePx;

    sf::RenderStates tilesetStates;
    tilesetStates.texture = &textures.tileset;
    window.draw(textures.floorBuffer, tilesetStates);

    // Stone path: each cell is a 2x2 of half-tile slabs, chosen per (cell, quadrant).
    if (!path.empty()) {
        const float half = tileSizePx / 2.0f;
        std::vector<sf::Vertex> vertices;
        vertices.reserve(path.size() * 4 * 6);
        for (const CellCoord& cell : path) {
            for (unsigned q = 0; q < 4; ++q) {
                const sf::IntRect& piece = textures.stoneTiles[cellHash(cell.x, cell.y, q + 1) % textures.stoneTiles.size()];
                const sf::Vector2f origin(cell.x * tileSizePx + static_cast<float>(q % 2) * half,
                                           cell.y * tileSizePx + static_cast<float>(q / 2) * half);
                appendQuad(vertices, origin, half, piece);
            }
        }
        window.draw(vertices.data(), vertices.size(), sf::PrimitiveType::Triangles, tilesetStates);
    }

    drawCoarseGrid(window, grid, tileSizePx);
    drawMarker(window, grid.spawnCell(), tileSizePx, sf::Color(80, 220, 120));
    drawMarker(window, grid.castleCell(), tileSizePx, sf::Color(230, 190, 60));

    // Walls.
    {
        auto view = registry.view<WallTag, Position>();
        for (auto entity : view) {
            const auto& pos = view.get<Position>(entity);
            drawSpriteCentered(window, textures.wall, sf::Vector2f(pos.x * tileSizePx, pos.y * tileSizePx), tileSizePx);
        }
    }

    // Towers.
    {
        auto view = registry.view<TowerState, Position>();
        for (auto entity : view) {
            const auto& pos = view.get<Position>(entity);
            drawSpriteCentered(window, textures.tower, sf::Vector2f(pos.x * tileSizePx, pos.y * tileSizePx), tileSizePx);
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

    // Enemies + health bars. The sprites are taller than a tile, so draw them back to
    // front (top of the screen first) to keep overlaps looking right.
    {
        auto view = registry.view<EnemyTag, Position, Health>();
        std::vector<entt::entity> enemies(view.begin(), view.end());
        std::sort(enemies.begin(), enemies.end(), [&](entt::entity a, entt::entity b) {
            return view.get<Position>(a).y < view.get<Position>(b).y;
        });

        const float scale = tileSizePx / static_cast<float>(kEnemyFrame.size.x);
        for (auto entity : enemies) {
            const auto& pos = view.get<Position>(entity);
            const auto& health = view.get<Health>(entity);

            const float feetY = (pos.y + kEnemyFeetOffsetTiles) * tileSizePx;
            sf::Sprite sprite(textures.enemy, kEnemyFrame);
            sprite.setOrigin(sf::Vector2f(static_cast<float>(kEnemyFrame.size.x) / 2.0f, kEnemyFeetRowPx));
            sprite.setPosition(sf::Vector2f(pos.x * tileSizePx, feetY));
            sprite.setScale(sf::Vector2f(scale, scale));
            window.draw(sprite);

            const float headY = feetY - (kEnemyFeetRowPx - kEnemyHeadRowPx) * scale;
            const float barY = headY - 6.0f;
            const float barWidth = tileSizePx * 0.7f;
            const float healthFrac = health.max > 0.0f ? std::max(0.0f, health.current / health.max) : 0.0f;

            sf::RectangleShape barBack(sf::Vector2f(barWidth, 3.0f));
            barBack.setOrigin(sf::Vector2f(barWidth / 2.0f, 0.0f));
            barBack.setPosition(sf::Vector2f(pos.x * tileSizePx, barY));
            barBack.setFillColor(sf::Color(60, 60, 60));
            window.draw(barBack);

            sf::RectangleShape barFront(sf::Vector2f(barWidth * healthFrac, 3.0f));
            barFront.setPosition(sf::Vector2f(pos.x * tileSizePx - barWidth / 2.0f, barY));
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
