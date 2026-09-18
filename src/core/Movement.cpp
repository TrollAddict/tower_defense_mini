#include "core/Movement.hpp"

#include <algorithm>
#include <cmath>

namespace td {

namespace {
constexpr float kCornerSkin = 0.001f; // tiles; keeps a clamped axis from re-crossing next frame
}

bool isBlockedForMovement(const Grid& grid, int x, int y) {
    return !grid.inBounds(x, y) || Grid::blocksMovement(grid.at(x, y));
}

void resolveCornerClipping(const Grid& grid, int oldCellX, int oldCellY, float& newX, float& newY) {
    const int tentativeCellX = std::clamp(static_cast<int>(std::floor(newX)), 0, grid.width() - 1);
    const int tentativeCellY = std::clamp(static_cast<int>(std::floor(newY)), 0, grid.height() - 1);

    if (tentativeCellX != oldCellX && tentativeCellY == oldCellY &&
        isBlockedForMovement(grid, tentativeCellX, oldCellY)) {
        newX = (tentativeCellX > oldCellX) ? (static_cast<float>(oldCellX + 1) - kCornerSkin)
                                            : static_cast<float>(oldCellX);
    }
    if (tentativeCellY != oldCellY && tentativeCellX == oldCellX &&
        isBlockedForMovement(grid, oldCellX, tentativeCellY)) {
        newY = (tentativeCellY > oldCellY) ? (static_cast<float>(oldCellY + 1) - kCornerSkin)
                                            : static_cast<float>(oldCellY);
    }
}

} // namespace td
