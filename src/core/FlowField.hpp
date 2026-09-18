#pragma once

#include <cstdint>
#include <limits>
#include <vector>

#include "core/Grid.hpp"

namespace td {

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
};

constexpr int kUnreachable = std::numeric_limits<int>::max();

// Single flow field, BFS outward from the castle (GDD §10 — one field total, not one
// per size class, since there is exactly one enemy footprint). Diagonal movement is
// allowed (GDD §7), so the BFS is 8-directional.
class FlowField {
public:
    // Full recompute every call — cheap enough at 256x256 to run synchronously on
    // every placement/removal (see GDD §10's cell-budget math). No dirty-region/
    // incremental BFS.
    void compute(const Grid& grid);

    bool isReachable(int x, int y) const;
    int distance(int x, int y) const;

    // Direction an entity standing at (x, y) should move to make progress toward the
    // castle. Zero vector if unreachable or already at the castle.
    Vec2 direction(int x, int y) const;

    // The cells an enemy starting at (x, y) walks through, following direction() one
    // cell at a time down to the castle (both ends included). Empty if (x, y) is
    // unreachable.
    std::vector<CellCoord> pathFrom(int x, int y) const;

private:
    int width_ = 0;
    int height_ = 0;
    std::vector<int> distance_;

    // Offset to the lowest-distance neighbor, or {0, 0} if there is none (castle or
    // unreachable).
    std::pair<int, int> bestStep(int x, int y) const;

    int index(int x, int y) const { return y * width_ + x; }
    bool inBounds(int x, int y) const { return x >= 0 && y >= 0 && x < width_ && y < height_; }
};

} // namespace td
