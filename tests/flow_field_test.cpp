// Smoke tests for the core pathfinding pipeline (GDD core pillar #1: "pathfinding is
// the whole game"). No framework dependency on purpose -- this project's only other
// dependencies are entt/nlohmann-json/SFML, and pulling in a test framework for four
// checks isn't worth it.
#include <algorithm>
#include <cmath>
#include <iostream>

#include "core/FlowField.hpp"
#include "core/Grid.hpp"
#include "core/Movement.hpp"

namespace {

int failures = 0;

void check(bool condition, const char* description) {
    if (!condition) {
        std::cerr << "FAILED: " << description << "\n";
        ++failures;
    } else {
        std::cout << "ok: " << description << "\n";
    }
}

void testOpenGridFullyReachable() {
    td::Grid grid(10, 10, td::CellCoord{0, 5}, td::CellCoord{9, 5});
    td::FlowField field;
    field.compute(grid);

    check(field.isReachable(grid.spawnCell().x, grid.spawnCell().y), "open grid: spawn reachable from castle BFS");
    check(field.isReachable(0, 0), "open grid: far corner reachable");
    check(field.distance(9, 5) == 0, "open grid: castle cell has distance 0 from itself");
}

// This is the exact situation GDD §7's place-then-verify-then-revert rule exists to
// catch: a wall placed across the only corridor seals the castle off from the spawn.
void testFullWallSealsPath() {
    td::Grid grid(5, 5, td::CellCoord{0, 2}, td::CellCoord{4, 2});
    for (int y = 0; y < 5; ++y) {
        grid.setCell(2, y, td::CellType::Wall);
    }

    td::FlowField field;
    field.compute(grid);

    check(!field.isReachable(grid.spawnCell().x, grid.spawnCell().y),
          "full-column wall: spawn is unreachable from castle (would trigger a revert)");
}

// GDD §7: diagonal movement is allowed with no corner-cutting restriction -- two
// diagonally-adjacent blockers should not, on their own, seal a path.
void testDiagonalGapNotBlocked() {
    td::Grid grid(5, 5, td::CellCoord{0, 0}, td::CellCoord{4, 4});
    // Blockers at (2,1) and (1,2) leave a diagonal-only gap between (1,1) and (2,2).
    grid.setCell(2, 1, td::CellType::Wall);
    grid.setCell(1, 2, td::CellType::Wall);

    td::FlowField field;
    field.compute(grid);

    check(field.isReachable(grid.spawnCell().x, grid.spawnCell().y),
          "diagonal gap: spawn still reachable through a diagonally-adjacent pair of blockers");
}

// Matches the actual game's layout (Game::startNewRun): 256x256, spawn in the
// top-left corner, castle in the bottom-right. Corner placement means each has only
// 3 in-bounds neighbors instead of an edge cell's 5 or an interior cell's 8 -- worth
// covering explicitly rather than assuming the edge-cell tests above generalize.
void testCornerToCornerAtFullScale() {
    td::Grid grid(256, 256, td::CellCoord{0, 0}, td::CellCoord{255, 255});
    td::FlowField field;
    field.compute(grid);

    check(field.isReachable(0, 0), "full-scale corners: spawn (top-left) reachable from castle BFS");
    check(field.distance(255, 255) == 0, "full-scale corners: castle cell has distance 0 from itself");
    check(field.distance(0, 0) == 255, "full-scale corners: corner-to-corner distance is 255 (pure diagonal path)");

    check(!grid.isBuildable(0, 0), "full-scale corners: spawn cell (top-left) rejects placement");
    check(!grid.isBuildable(255, 255), "full-scale corners: castle cell (bottom-right) rejects placement");
}

// Regression test for a bug where an enemy would visibly freeze wedged between two
// diagonally-placed structures. Root cause: continuous (non-grid-locked) movement
// adds dir.x*speed*dt and dir.y*speed*dt independently, so when squeezing through a
// diagonal-only gap, one axis can cross its cell boundary before the other -- landing
// the sampled cell inside a flanking wall for a frame, where FlowField has no
// direction defined, permanently zeroing velocity. This simulates real per-frame
// movement (the same math MovementSystem::updateMovement does) starting from a
// deliberately asymmetric fractional position -- the exact condition that triggers
// the bug -- and asserts the entity still crosses into the open diagonal cell instead
// of freezing at the corner.
void testCornerClippingDoesNotFreezeMovement() {
    td::Grid grid(6, 6, td::CellCoord{0, 0}, td::CellCoord{5, 5});
    // Walls at (3,2) and (2,3) leave a diagonal-only gap between (2,2) and (3,3).
    grid.setCell(3, 2, td::CellType::Wall);
    grid.setCell(2, 3, td::CellType::Wall);

    td::FlowField field;
    field.compute(grid);
    check(field.isReachable(0, 0), "corner-clip regression: spawn reachable through the diagonal gap");

    // Asymmetric fractional position inside cell (2,2): y is much closer to its
    // boundary than x, so y crosses into row 3 (the wall's row) long before x
    // crosses into column 3 -- this is exactly what used to freeze movement.
    float x = 2.1f;
    float y = 2.9f;
    const float speed = 1.5f;
    const float dt = 1.0f / 60.0f;

    bool crossedTheGap = false;
    for (int frame = 0; frame < 600 && !crossedTheGap; ++frame) {
        const int cellX = std::clamp(static_cast<int>(std::floor(x)), 0, grid.width() - 1);
        const int cellY = std::clamp(static_cast<int>(std::floor(y)), 0, grid.height() - 1);
        const td::Vec2 dir = field.direction(cellX, cellY);

        float newX = x + dir.x * speed * dt;
        float newY = y + dir.y * speed * dt;
        td::resolveCornerClipping(grid, cellX, cellY, newX, newY);
        x = newX;
        y = newY;

        if (x >= 3.0f && y >= 3.0f) {
            crossedTheGap = true;
        }
    }

    check(crossedTheGap, "corner-clip regression: entity crosses the diagonal gap within 10s instead of freezing");
}

void testSpawnAndCastleCellsNotBuildable() {
    td::Grid grid(8, 8, td::CellCoord{0, 4}, td::CellCoord{7, 4});
    check(!grid.isBuildable(0, 4), "spawn cell rejects placement");
    check(!grid.isBuildable(7, 4), "castle cell rejects placement");
    check(grid.isBuildable(3, 3), "an ordinary open cell accepts placement");

    check(grid.setCell(3, 3, td::CellType::Tower), "placing a tower on an open cell succeeds");
    check(!grid.isBuildable(3, 3), "an occupied cell rejects a second placement");
}

} // namespace

int main() {
    testOpenGridFullyReachable();
    testFullWallSealsPath();
    testDiagonalGapNotBlocked();
    testCornerToCornerAtFullScale();
    testCornerClippingDoesNotFreezeMovement();
    testSpawnAndCastleCellsNotBuildable();

    if (failures > 0) {
        std::cerr << failures << " check(s) failed\n";
        return 1;
    }
    std::cout << "All checks passed\n";
    return 0;
}
