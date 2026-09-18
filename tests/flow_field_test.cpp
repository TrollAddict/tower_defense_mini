// Smoke tests for the core pathfinding pipeline (GDD core pillar #1: "pathfinding is
// the whole game"). No framework dependency on purpose -- this project's only other
// dependencies are entt/nlohmann-json/SFML, and pulling in a test framework for four
// checks isn't worth it.
#include <iostream>

#include "core/FlowField.hpp"
#include "core/Grid.hpp"

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
    testSpawnAndCastleCellsNotBuildable();

    if (failures > 0) {
        std::cerr << failures << " check(s) failed\n";
        return 1;
    }
    std::cout << "All checks passed\n";
    return 0;
}
