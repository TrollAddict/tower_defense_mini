#include "core/FlowField.hpp"

#include <array>
#include <cmath>
#include <queue>

namespace td {

namespace {
// 8-directional neighbor offsets. Diagonal movement is allowed with no corner-cutting
// restriction (GDD §7): a unit can cut through a diagonal gap even if both orthogonal
// neighbors are blocked, so BFS simply expands to any walkable 8-neighbor.
constexpr std::array<std::pair<int, int>, 8> kOffsets{{
    {1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1},
}};
} // namespace

void FlowField::compute(const Grid& grid) {
    width_ = grid.width();
    height_ = grid.height();
    const std::size_t cellCount = static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_);
    distance_.assign(cellCount, kUnreachable);

    auto walkable = [&](int x, int y) {
        if (!inBounds(x, y)) return false;
        return !Grid::blocksMovement(grid.at(x, y));
    };

    const CellCoord& castle = grid.castleCell();
    std::queue<CellCoord> frontier;
    distance_[index(castle.x, castle.y)] = 0;
    frontier.push(castle);

    while (!frontier.empty()) {
        CellCoord current = frontier.front();
        frontier.pop();
        const int currentDist = distance_[index(current.x, current.y)];

        for (const auto& [dx, dy] : kOffsets) {
            const int nx = current.x + dx;
            const int ny = current.y + dy;
            if (!walkable(nx, ny)) continue;
            const int idx = index(nx, ny);
            if (distance_[idx] != kUnreachable) continue;
            distance_[idx] = currentDist + 1;
            frontier.push(CellCoord{nx, ny});
        }
    }
}

bool FlowField::isReachable(int x, int y) const {
    if (!inBounds(x, y)) return false;
    return distance_[index(x, y)] != kUnreachable;
}

int FlowField::distance(int x, int y) const {
    if (!inBounds(x, y)) return kUnreachable;
    return distance_[index(x, y)];
}

std::pair<int, int> FlowField::bestStep(int x, int y) const {
    if (!inBounds(x, y)) return {0, 0};
    const int myDist = distance_[index(x, y)];
    if (myDist == kUnreachable || myDist == 0) return {0, 0};

    int bestDist = myDist;
    std::pair<int, int> best{0, 0};
    for (const auto& [dx, dy] : kOffsets) {
        const int nx = x + dx;
        const int ny = y + dy;
        if (!inBounds(nx, ny)) continue;
        const int nd = distance_[index(nx, ny)];
        if (nd == kUnreachable) continue;
        if (nd < bestDist) {
            bestDist = nd;
            best = {dx, dy};
        }
    }
    return best;
}

Vec2 FlowField::direction(int x, int y) const {
    const auto [bestDx, bestDy] = bestStep(x, y);
    if (bestDx == 0 && bestDy == 0) return Vec2{0.0f, 0.0f};
    const float len = std::sqrt(static_cast<float>(bestDx * bestDx + bestDy * bestDy));
    return Vec2{static_cast<float>(bestDx) / len, static_cast<float>(bestDy) / len};
}

std::vector<CellCoord> FlowField::pathFrom(int x, int y) const {
    std::vector<CellCoord> path;
    if (!isReachable(x, y)) return path;
    path.push_back(CellCoord{x, y});
    while (true) {
        const auto [dx, dy] = bestStep(x, y);
        if (dx == 0 && dy == 0) break;
        x += dx;
        y += dy;
        path.push_back(CellCoord{x, y});
    }
    return path;
}

} // namespace td
