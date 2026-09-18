#include "core/Grid.hpp"

namespace td {

Grid::Grid(int width, int height, CellCoord spawnCell, CellCoord castleCell)
    : width_(width),
      height_(height),
      spawn_(spawnCell),
      castle_(castleCell),
      cells_(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), CellType::Empty) {}

bool Grid::inBounds(int x, int y) const {
    return x >= 0 && y >= 0 && x < width_ && y < height_;
}

bool Grid::isBuildable(int x, int y) const {
    if (!inBounds(x, y)) return false;
    if (CellCoord{x, y} == spawn_ || CellCoord{x, y} == castle_) return false;
    return cells_[index(x, y)] == CellType::Empty;
}

CellType Grid::at(int x, int y) const {
    if (!inBounds(x, y)) return CellType::Empty;
    return cells_[index(x, y)];
}

bool Grid::setCell(int x, int y, CellType type) {
    if (!isBuildable(x, y)) return false;
    cells_[index(x, y)] = type;
    return true;
}

void Grid::clearCell(int x, int y) {
    if (!inBounds(x, y)) return;
    cells_[index(x, y)] = CellType::Empty;
}

bool Grid::blocksMovement(CellType type) {
    return type == CellType::Tower || type == CellType::Wall;
}

} // namespace td
