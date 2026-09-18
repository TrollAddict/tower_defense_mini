# Tower Defense Mini

A minimal tower defense: one 256x256 map, one enemy type, one tower type, one wall
type. Enemies flow toward your castle along a single shared flow field; you place
towers and walls to bend their route. A placement that would seal the castle off is
automatically reverted and refunded -- see
[`docs/GAME_DESIGN_DOCUMENT.md`](docs/GAME_DESIGN_DOCUMENT.md) for the full design
rationale, and its parent doc
[`tower_defense_pathfinder`](../tower_defense_pathfinder/docs/GAME_DESIGN_DOCUMENT.md)
for the larger game this is a scoped-down slice of.

## Building

Requires CMake 3.24+, a C++20 compiler, and an internet connection on first configure
(SFML, EnTT, and nlohmann-json are fetched via `FetchContent` -- there are no system
package dependencies to install beyond SFML's own native libs: X11/GL/freetype/etc.,
which most desktop Linux installs already have).

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

The first build compiles SFML from source and takes several minutes; subsequent
builds are incremental.

Run the game:

```sh
./build/tower_defense_mini
```

Run the pathfinding smoke tests:

```sh
ctest --test-dir build --output-on-failure
```

## Controls

- **Main menu:** drag the three sliders to set difficulty (wave size / spawn rate /
  enemy health multipliers, 1-99), then click Start.
- **In game:**
  - `1` / `2` -- select Tower / Wall to place
  - Left click -- place the selected structure at the cell under the cursor
  - Right click -- remove whatever's at the cell under the cursor
  - `WASD` / arrow keys -- pan the camera
  - Mouse wheel -- zoom
  - `Home` -- zoom out to fit the whole map
  - `Enter` -- skip the current intermission (including the initial 30s build phase)
    and start the next wave immediately
  - `Esc` -- abandon the run and return to the main menu

## Project layout

- `src/core/` -- SFML-free simulation core: `Grid` (occupancy), `FlowField` (BFS
  pathing), `Config` (JSON data loading). This is what `tests/` exercises directly.
- `src/ecs/` -- ECS component definitions (entt).
- `src/game/` -- gameplay systems (combat, movement, waves, placement) and the
  SFML-facing `Game`/`RenderSystem`.
- `data/*.json` -- tower/wall/enemy/economy/wave/difficulty numbers. Edit these to
  rebalance without touching code (GDD §11).
- `docs/GAME_DESIGN_DOCUMENT.md` -- the design doc this build implements.

## Known gaps vs. the GDD

- **Save/load is not implemented.** The GDD's technical architecture section
  (§11) calls for JSON save/load; it isn't part of the §16 MVP definition, so it was
  left out of this first build. The map is also fixed/hand-authored (spawn point in
  the top-left corner, castle in the bottom-right), so there's no persistent run to
  lose between sessions yet.
- **No live placement preview** (GDD §7 calls this optional QoL at this scope, since
  the real recompute is cheap enough to just run on every placement and revert on
  failure). Currently the only feedback on a rejected placement is the on-screen
  notification after the fact.
- **Wall shape restriction** from the parent doc was dropped, per the GDD's own
  `DEFAULT` recommendation (walls can't be attacked here, so the original
  damage-soaking-block concern doesn't apply).
