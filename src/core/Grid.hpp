#pragma once

#include <cstdint>
#include <vector>

namespace td {

enum class CellType : std::uint8_t {
    Empty = 0,
    Tower = 1,
    Wall = 2,
};

struct CellCoord {
    int x = 0;
    int y = 0;

    bool operator==(const CellCoord& other) const {
        return x == other.x && y == other.y;
    }
};

// Single flat, unchunked grid (see GDD §11 — 256x256 is small enough that chunking,
// which the parent project needs for large-map streaming, isn't warranted here).
class Grid {
public:
    Grid(int width, int height, CellCoord spawnCell, CellCoord castleCell);

    int width() const { return width_; }
    int height() const { return height_; }
    const CellCoord& spawnCell() const { return spawn_; }
    const CellCoord& castleCell() const { return castle_; }

    bool inBounds(int x, int y) const;
    bool isBuildable(int x, int y) const;
    CellType at(int x, int y) const;

    // Returns false (no-op) if the cell is out of bounds, already occupied, or is the
    // spawn/castle cell. Caller (PlacementController) is responsible for the
    // reachability check and revert — this just mutates occupancy.
    bool setCell(int x, int y, CellType type);
    void clearCell(int x, int y);

    // Whether `type` blocks pathing (Tower and Wall both do; only the structure
    // footprint differs from the parent doc, not the blocking rule).
    static bool blocksMovement(CellType type);

private:
    int width_;
    int height_;
    CellCoord spawn_;
    CellCoord castle_;
    std::vector<CellType> cells_;

    int index(int x, int y) const { return y * width_ + x; }
};

} // namespace td
