# Game Design Document — Tower Defense Mini

> Companion/prototype scope to `tower_defense_pathfinder`'s
> [GAME_DESIGN_DOCUMENT.md](../../tower_defense_pathfinder/docs/GAME_DESIGN_DOCUMENT.md)
> ("Enigmatology Defense"). Same technical foundation (ECS, C++, SFML, grid-based forced
> pathing), deliberately tiny content scope: **1 map size, 1 enemy type, 1 tower type, 1
> wall type.** Where a decision here diverges from the parent doc, the diff and the reason
> are called out explicitly so this reads as "the same architecture, radically smaller
> surface area" rather than a different game. Sections marked `TODO` need decisions.
> Sections marked `DEFAULT` are a suggested starting point — replace or confirm.

---

## 1. Vision

- **Elevator pitch:** A minimal tower defense: one map, one enemy, one tower, one wall.
  Enemies stream toward your castle along a flow field; you place towers and walls to
  bend their path into a kill lane. The whole game *is* the pathfinding puzzle — no
  economy sprawl, no tech tree, no size classes to learn.
- **Core fantasy:** "Draw a maze with two pieces and watch it work." Build small, watch
  the path update live, don't over-seal it.
- **Reference games:** Same lineage as the parent doc (Sunken Defense-style forced
  pathing) but reduced toward something closer to a classic single-lane/open-field tower
  defense — the maze-design skill expression survives with just 1 wall + 1 tower type,
  the base-building/economy/horde-scale layers do not.
- **Relationship to `tower_defense_pathfinder`:** this is not a different game so much as
  the smallest possible slice of the same one — same ECS/grid/flow-field core, a fraction
  of the content and none of the systems (economy depth, tech tree, size classes, fog of
  war, attention/attack-mode AI) that make the parent doc's scope large. Good for
  validating the pathfinding pipeline end-to-end before the parent game's content scope
  is built on top of it.
- **What we are NOT building:** no multiplayer, no roguelite meta-progression, no fog of
  war, no biomes/terrain variety, no resource-gathering economy beyond a single simple
  currency (§8), no tech tree, no enemy/tower/wall size classes, no attention/attack-mode
  mechanic (§5, §7 — see why below), no chunked/streaming map.

## 2. Core Pillars

1. **Pathfinding is the whole game.** With exactly one tower and one wall, the only
   decision space left is geometry — where you place things. The flow field must be
   correct, visibly reactive, and fast, because it's not sharing the spotlight with an
   economy or tech tree.
2. **Placement can never break the game.** The parent doc handles a sealed-off path by
   punishing the player (attention timer → attack mode → escalating damage on re-seal).
   This game has no attention mechanic, so it needs a different safety net: **if a
   placement would leave the castle unreachable, that placement is immediately reverted**
   (§7). The player is free to experiment without ever soft-locking the level.
3. **Small enough to hold in your head.** 256x256 is large enough to be a genuine
   pathfinding stress case (see §10's math) but small enough that the whole map, and the
   consequence of any one placement, is visible without a minimap or fog of war.
4. **No moving parts beyond placement + pathing + combat.** No economy balancing, no
   upgrade trees, no size-class matrix. This exists to prove the core tech
   (grid → flow field → dynamic-obstacle recompute → rendering) cleanly before the parent
   game's much larger content scope sits on top of it.

## 3. Gameplay Loop

- **Session shape:** endless waves, single map, no mission structure.
- **Macro loop:** place towers/walls to shape the route from the spawn point to the
  castle → survive escalating waves of the one enemy type → earn currency from kills →
  place more structures → repeat until the castle is destroyed.
- **Micro loop (moment to moment):** watch enemies flow along the current path, place or
  remove a wall/tower to reroute or choke them, confirm the path is still valid (auto-
  enforced, see §7), watch towers shoot enemies as they pass in range.
- **Win condition(s):** none in the MVP — this is a survive-as-long-as-possible sandbox,
  not a mission with a win state.
- **Lose condition(s):** castle HP reaches 0 (an enemy reaching the castle damages it,
  then despawns).
- **Session length target:** 10-30 min.

## 4. World & Map

- **Map size:** **256x256 (decided, per brief).** See §10 for why this size needs none
  of the parent doc's chunking/incremental-BFS/dirty-region machinery — a full recompute
  every placement is cheap at this cell count.
- **Tile system:** grid-based. 32px tiles (8192x8192px world) as a screen-friendly
  starting point; revisit once art/camera are in.
- **Fog of war / vision:** none. Whole map always visible — there's only one spawn point
  and one castle, nothing to obscure that adds gameplay value at this scope.
- **Biomes / terrain types:** none. Flat buildable grid.
- **Terrain generation:** hand-authored single layout: one spawn point, one castle, open
  buildable field between them.
- **Resource nodes:** none — see §8, currency comes from kills, not gathering.
- **Points of interest:** exactly two: the spawn point (enemies enter here) and the
  castle (enemies path toward this; player loses when it's destroyed).
- **Expansion model:** none — no territory/influence system, the whole grid is buildable
  from the start.

## 5. Enemy Design

**No attention/attack-mode mechanic** (this is the headline difference from the parent
doc's §5). In the parent game, enemies that fail to make forward progress for too long
switch to attacking the nearest structure, which is how a sealed maze gets punished
instead of soft-locking the game. This game has no such state: enemies only ever walk the
current flow field toward the castle and take tower damage while doing so. The reason
this is safe here is the §7 rule — a placement that would seal the path off is reverted
before it ever takes effect, so "enemy stuck with no path" is a state that should never
occur. A consequence worth calling out: **enemies can never damage or destroy a tower or
wall** — there's no attack behavior defined for them at all, so structures are permanent
once placed until the player removes them.

- **Enemy count target:**  a few hundred simultaneous active enemies at 60 FPS is a reasonable target for a single-lane,
  single-type mini game; revisit once the loop is playable.
- **Enemy archetype (the only one):**
  - Walker — moderate speed, moderate HP, 1x1 footprint. No variants.
- **Unit size / footprint classes:** none needed — with one enemy type there's only one
  footprint, so the parent doc's per-size-class flow fields collapse to a single field
  (see §10).
- **Spawn model:** single spawn point, wave-based; enemy count per wave escalates over
  time (§14).
- **Escalation curve:** Every wave spawns more enemies than the last, rounded down,
  by a percentage read from config (`data/waves.json`'s `growth_rate_per_wave`,
  currently 25% -- not the originally-specified 10%: at `base_enemy_count` 5,
  `floor(5 * 1.10) == 5` is a fixed point, so 10% growth never actually grew the
  count at all. The implementation also guarantees at least +1 per wave regardless of
  the configured rate, so this can't silently reoccur at some other base-count/rate
  combination later.)
- **Aggro / targeting rules:** always path toward the castle via the current flow field.
  No alternate targeting state.
- **Death/cleanup behavior:** despawn immediately on death (pooled, no corpse/decal
  system needed at this scope — can be added later without affecting the pathing core).

## 6. Towers & Walls

- **Placement model:** grid-snapped, 1x1 footprint for both the tower and the wall —
  same as the parent doc's footprint rule, just with no larger footprints to support
  since there's only one size of each structure.
- **Tower (the only one):** basic single-target
  - range : 2 tiles
  - damage : 2 damage
  - attack_speed : 1 per second
  - cost : 5
- **Wall (the only one):** pure blocker, no attack of its own. Costs currency to place.
  Since enemies never attack structures (§5), **the wall needs no HP/durability stat** —
  it is destroyed only when the player removes it, never by combat. This is a real
  simplification versus the parent doc's tiered wood/stone/reinforced/metal wall HP
  system, and it's a direct consequence of cutting the attention/attack-mode mechanic,
  not an oversight.
- **Wall shape restriction:** `no shape restriction — any
  wall layout is legal as long as §7's reachability check still passes.
- **Upgrade model:** none — no leveling, no tech tree (§9).
- **Ammo/upkeep economy:** none — placement is a one-time currency cost, no per-shot or
  per-tick resource consumption.
- **Power/logistics grid:** none — no influence-radius requirement for towers to
  function.

## 7. Maze & Forced Pathing (Grid-Based Blocking)

This is the mechanic the whole mini-game exists to exercise, so its rule needs to be
precise. It is a deliberately different ruleset from the parent doc's, not just a smaller
version of it — see the callout below.

- **Grid & footprint rules:** every tower/wall occupies exactly one grid cell, 1:1 with
  the pathing grid (same principle as parent §7, simpler since there's only one footprint
  size in play).
- **Path validation rule — place-then-verify-then-possibly-revert:** when the player
  places a tower or wall, the game immediately recomputes reachability from the spawn
  point to the castle. If the castle is now unreachable, **that placement is
  automatically undone** (the structure is removed, its currency cost refunded) and the
  player gets a short notification (e.g. "No path — placement reverted"). This is the
  literal rule from the brief: *"if there is no path found then the player's last tower
  or wall is removed."*
  - Because this check runs after every single placement (not batched), "the last
    tower or wall" is unambiguous — it's always the one edit that was just made. There's
    no scenario where an earlier structure needs to be identified/removed instead.
  - **This replaces two separate parent-doc mechanics at once:** the parent game has no
    placement validation at all (sealing is *allowed*, then punished via the attention/
    attack-mode escalation in §5/§7) — this game has the opposite policy, no sealing is
    ever allowed to persist, enforced immediately rather than punished after the fact.
    That's why §5 has no attention mechanic and §6's wall has no HP: there is nothing
    left for an enemy to ever need to attack.
  - **Removal is always safe and needs no check:** removing a tower or wall can only
    open the graph up, never close it off, so removal never triggers a
    revalidate-or-revert cycle.
- **Live preview:** optional QoL, not load-bearing for correctness the way it is in the
  parent doc (where the full flow-field recompute is too slow to run per placement and a
  separate cheap check is needed for the preview). Here the real recompute is cheap
  enough (§10) to just run for real on every placement and revert if it fails — a
  "would this be valid" preview is a nice-to-have, not required. `DEFER`.
- **Diagonal movement / corner-cutting:** diagonal movement allowed, same as parent §7 —
  two diagonally-adjacent structures don't form a seal on their own, a player wanting a
  hard block needs an orthogonally-connected run.
- **Mid-combat editing:** freely allowed at any time, no builder-unit/construction-stage
  gating — that gating exists in the parent doc to support its mid-combat-editing-vs-
  bulldozing tension, which doesn't apply here since nothing can be bulldozed.
- **Sealing as a tactic:** not applicable — sealing is prevented outright by the
  place-then-verify rule above, so there's no escalating-punishment mechanic to design
  (contrast parent §7's "multiply attack power by seal count").
- **Multiple simultaneous entrances:** not applicable at this scope — one spawn point,
  one castle, one path graph.

## 8. Economy & Resources

- **Resource list:** one currency name, bone. No raw materials, no
  population, no tiers.
- **Income sources:** currency earned per enemy kill 1 ,with bonus being the number of waves completed. wave 1 gives 1 bonus at completion, wave 10 gives 10 bonus.
- **Spend sinks:** placing a tower or placing a wall tower cost: 5, wall cost: 3. No upkeep, no repair
  (nothing to repair — see §6), no tech.
- **Refund rule:** a placement that gets auto-reverted by §7 refunds its full cost — the
  player should never lose currency to an attempt that never actually took effect.
- **Balancing targets:** DEFER, playtesting will help with this. The currency values should all be in json items to allow easy configuration

## 9. Progression & Tech Tree

- **Meta-progression (between sessions):** none.
- **In-session tech tree:** none — cut entirely, unlike the parent doc's Diablo-2-style
  talent tree. With one tower and one wall there's nothing to gate behind research.
- **Difficulty/mission structure:** single sandbox map, endless waves (§3).

## 10. Pathfinding & AI (critical path — same core as the parent repo)

- **Enemy movement algorithm:** flow field / vector field pathfinding, **single field**
  (not per-size-class — there's only one enemy footprint, so the parent doc's "one field
  per size class per target" collapses to one field, period). BFS outward from the castle
  across the 256x256 grid, same direction-to-goal principle as parent §10.
- **Why 256x256 needs none of the parent doc's chunking/incremental-BFS work:** the
  parent doc sized its map at 1024x1024 specifically because a full-grid BFS restart on
  every edit was too slow at 2048x2048 (measured ~209 frames / ~3.5s to converge at its
  benchmarked `kFlowFieldCellBudgetPerFrame` of 20,000 cells/frame). Applying that same
  budget here: 256x256 = 65,536 cells ÷ 20,000 cells/frame ≈ **4 frames (~0.07s at
  60 FPS)** to fully reconverge from scratch on every single placement. That's fast
  enough to run a full recompute synchronously on every edit — no dirty-region BFS, no
  amortization across frames, no chunked map (§11) needed at this scope. If the map size
  ever grows past a few hundred cells per side, revisit this.
- **Dynamic obstacles / recompute strategy:** full recompute, every edit (place, remove,
  or an auto-revert from §7), given the above. This recompute *is* the §7 validity check
  — reachability is read directly off the freshly computed field (`hasFlow(castleX,
  castleY)` equivalent), no separate fast-path BFS needed the way the parent doc requires
  for its build-preview.
- **Local avoidance:** accept single-file queuing in tight corridors, same as parent.
- **Grid resolution vs. performance:** pathing grid equals the build grid (256x256
  cells), same principle as parent §10.
- **Group/hive AI:** none — no attention/attack-mode state machine (§5), enemies simply
  follow the field.
- **Update frequency:** event-driven — recompute only fires on a structure add/remove,
  never on a timer.

## 11. Technical Architecture (C++ / SFML)

Same stack and architectural philosophy as the parent doc; several of its scale-driven
optimizations are unnecessary at this content scope and are called out as cut, not
forgotten.

- **Engine approach:** SFML for window/render/audio/input only; ImGui for dev/debug UI —
  unchanged from parent.
- **Architecture style:** data-oriented ECS (entt) — unchanged. Even at a few hundred
  enemies, ECS keeps this consistent with the parent codebase and avoids a rewrite when
  content scope grows later.
- **Grid/pathing data structure:** a single flat 256x256 grid storing per-cell
  walkability + occupying structure. No chunking (parent doc chunks specifically to
  stream/cull/update *large* maps regionally — 256x256 is small enough to hold and
  recompute in full every time, see §10).
- **Rendering strategy:** sprite batching (`sf::VertexArray` per texture) and frustum
  culling still apply and are cheap wins regardless of scale; texture atlasing and LOD
  are `DEFER` — the parent doc needs them for thousands of on-screen entities, this game
  likely does not at a few hundred.
- **Simulation/render split:** fixed-timestep simulation decoupled from render framerate
  — unchanged.
- **Multithreading:** `DEFER` — the parent doc parallelizes per-size-class flow field
  recompute, which doesn't apply here (single field); a single 256x256 BFS at ~4 frames
  worst case doesn't need to be split off the main thread to hit a 60 FPS budget. Revisit
  if profiling says otherwise.
- **Spatial partitioning:** uniform grid for tower-range/targeting queries — unchanged
  principle, smaller data.
- **Object pooling:** enemies and projectiles pooled, not allocated per spawn/death —
  unchanged.
- **Map data representation:** single unchunked tile grid (see above) — this is the
  clearest architectural simplification versus the parent doc.
- **Save/load:** JSON, serializing: map/structure state, all entities, current
  currency, current wave/spawn state, current path/maze layout. No fog of war, no tech
  state (both cut, §4/§9).
- **Config/data-driven design:** tower stat, enemy stat, and wave-escalation numbers
  should still live in external data (JSON), not hardcoded — same principle as parent
  §11, just far fewer files (one tower, one enemy, one wall).
- **Target platform(s) & min spec:** `parent targets Windows/Linux/Switch, 8GB
  RAM min; this game's footprint is much lighter so the same targets should be trivially
  achievable. If mobile doesn't require a complete re-write of architecture that would be a goal to add.

## 12. UI / UX

- **Camera:** pan/zoom, with a zoom-to-fit option to see the whole 256x256 map at once
  (no minimap needed at this scope — the whole map *is* the minimap).
- **HUD:** currency total, castle HP, current wave/kill counter.
- **Build UI:** two buttons (place tower / place wall), placement preview, range
  indicator for the tower. No grid-overlay/gap-width indicators needed the way the
  parent doc's multi-size-class system requires — there's only one gap-size question
  ("does a path exist at all"), and §7 answers it automatically rather than needing the
  player to reason about it visually before committing.
- **Notifications/alerts:** the one that matters — "No path, placement reverted" from
  §7. Also castle-under-attack / low-HP warning.

## 13. Audio

- **Music:** simple loop, no adaptive layering needed at this scope. `DEFER`.
- **SFX:** tower fire, enemy death, placement, placement-reverted, castle-hit. Voice
  count is naturally capped by the low tower/enemy count — no culling/priority scheme
  needed the way the parent doc's thousands-of-units case requires.
- **Ambient/horde audio:** none — no horde-scale dread cue needed at this population.

## 14. Difficulty & Balancing

- **Difficulty levers:** wave size growth rate, spawn interval, enemy health. All configurable in the main menu with slider menu's from 1-99 mutliplier.
- **Balancing philosophy:** since sealing is impossible (§7) and structures are
  permanent (§5/§6), the only tension is "can your tower DPS + maze length kill Walkers
  before they reach the castle" — a much narrower balancing space than the parent doc's
  attention-mechanic/size-class/economy interplay.
- **Playtesting metrics to track:** average FPS at target enemy count, currency
  surplus/deficit over time, how often placements get auto-reverted (a high revert rate
  probably means the player is fighting the UI, not making a deliberate maze choice —
  worth watching as a signal the live-preview QoL from §7 should get prioritized after
  all).

## 15. Art & Audio Style

- **Visual style:** pixel art to stay consistent with parent doc,
  but placeholder rectangles are fine for the pathfinding-proving MVP.
- **Color language:** reuse parent doc's cool-to-warm friendly-to-enemy
  gradient if/when real art lands; irrelevant to placeholder-rectangle prototyping.
- **Reference art:** `TODO`.

## 16. Scope & Roadmap

- **MVP definition:** this document *is* the MVP — there is no further slice to cut. 1
  map (256x256), 1 enemy type, 1 tower type, 1 wall type, single flow-field pathing,
  place-then-verify-then-revert path safety net, simple currency economy, no tech tree,
  no fog of war, no attention/attack-mode AI.
- **Milestones:** prototype (grid + single flow field + placement/revert rule working,
  placeholder art) → playable loop (waves, currency, tower combat tuned) → polish (real
  art/audio, `TODO` from §13/§15).
- **Biggest technical risks:** low, by design — the one worth tracking is confirming the
  §10 recompute-cost math (4 frames / ~0.07s) holds up once real engine overhead
  (rendering, ECS iteration, entt) is added on top of the raw BFS cost, not just the BFS
  in isolation.
- **Biggest design risks:** the game might be *too* simple to hold attention once the
  novelty of the flow field wears off, since there's no maze-shape depth beyond "one
  wall type, no shape restriction, can't be attacked." Worth watching in playtesting
  before deciding whether a second tower or wall type is worth the scope increase.

## 17. Open Questions / Next TODOs

1. **Art direction (§15)** — placeholder rectangles vs. early pixel-art pass.
