#pragma once

#include "core/Grid.hpp"

namespace td {

bool isBlockedForMovement(const Grid& grid, int x, int y);

// GDD §7 allows diagonal movement through a single open corner point even when both
// flanking cells are walls ("two diagonally-adjacent blockers do not actually block a
// determined unit"). With continuous (non-grid-locked) positions, naively adding
// dir.x*speed*dt and dir.y*speed*dt independently lets one axis cross its cell
// boundary before the other -- e.g. pos.x reaches the next column while pos.y is
// still in the old row -- which means the entity's sampled cell for that frame (and
// every frame after, since it never moves again) is the FLANKING WALL cell, not the
// open diagonal cell. A wall cell has no flow direction, so movement permanently
// zeroes out there: the entity looks physically wedged between the two structures.
// This clamps whichever axis would cross alone back to the cell boundary until the
// other axis catches up, so the two axes only ever cross together, straight through
// the corner point -- preserving the "diagonal squeeze is legal" design while never
// letting a sampled cell land inside a wall.
void resolveCornerClipping(const Grid& grid, int oldCellX, int oldCellY, float& newX, float& newY);

} // namespace td
