// Coverage for WaveDirector::giveUp() -- the "GIVE UP" action that converts a
// straggler-dominated wave's remaining enemies straight into castle damage instead of
// making the player wait for them to walk the rest of a (possibly very long) path.
// Drives WaveDirector directly rather than through Game/RenderSystem, which is why
// GameState/WaveDirector moved into td_mini_core (see CMakeLists.txt) -- neither
// needs SFML.
#include <iostream>

#include <entt/entt.hpp>

#include "core/Config.hpp"
#include "core/Grid.hpp"
#include "ecs/Components.hpp"
#include "game/GameState.hpp"
#include "game/WaveDirector.hpp"

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

} // namespace

int main() {
    td::GameConfig config; // struct defaults from Config.hpp -- no JSON files needed
    config.waves.baseEnemyCount = 4;
    config.waves.spawnIntervalSeconds = 1.0f;
    config.enemy.castleDamage = 3.0f;

    td::Grid grid(10, 10, td::CellCoord{0, 5}, td::CellCoord{9, 5});
    entt::registry registry;
    td::GameState state(config);
    td::WaveDirector director(grid, registry, state, config);

    check(director.giveUp() == 0, "give up: no-op during intermission -- nothing active to give up on");
    check(!director.hasActiveNotification(), "give up: a no-op doesn't set a notification");

    director.skipIntermission();
    director.update(0.001f); // this call only transitions Intermission -> Spawning; nothing spawns yet
    check(director.enemiesRemainingToSpawn() == 4, "give up setup: entering Spawning doesn't spawn anything itself");

    director.update(0.001f); // Spawning's own logic now runs: spawnTimer_ started at 0, so enemy #1 spawns
    check(director.enemiesRemainingToSpawn() == 3, "give up setup: wave 1's first enemy spawned");

    director.update(2.0f); // dt exceeds spawnIntervalSeconds -- spawns enemy #2
    check(director.enemiesRemainingToSpawn() == 2, "give up setup: a second enemy spawned");
    check(registry.view<td::EnemyTag>().size() == 2, "give up setup: exactly 2 enemy entities exist so far");

    const float castleHealthBefore = state.castleHealth();
    const int currencyBefore = state.currency();
    const int converted = director.giveUp();

    check(converted == 2, "give up: converts exactly the currently-alive stragglers (2), not the full wave count (4)");
    check(state.castleHealth() == castleHealthBefore - 2.0f * config.enemy.castleDamage,
          "give up: castle takes damage once per converted straggler");
    check(registry.view<td::EnemyTag>().size() == 0, "give up: every straggler entity is actually destroyed");
    check(director.enemiesRemainingToSpawn() == 0, "give up: cancels the rest of the wave's unspawned enemies too");
    check(director.isInIntermission(), "give up: immediately transitions to intermission");
    check(state.currency() == currencyBefore + 1, "give up: still awards the wave-clear currency bonus (wave 1 -> +1)");
    check(director.hasActiveNotification(), "give up: sets a HUD notification");

    if (failures > 0) {
        std::cerr << failures << " check(s) failed\n";
        return 1;
    }
    std::cout << "All checks passed\n";
    return 0;
}
