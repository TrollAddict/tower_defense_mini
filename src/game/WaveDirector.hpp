#pragma once

#include <entt/entt.hpp>

#include "core/Config.hpp"
#include "core/Grid.hpp"
#include "game/GameState.hpp"

namespace td {

// Drives endless wave spawning (GDD §3/§5/§14). Wave N's enemy count grows 10% off
// wave N-1's, rounded down (GDD §5's escalation curve), then scaled by the player's
// difficulty multiplier. A wave is "cleared" when every enemy from it has either died
// or reached the castle; clearing awards a currency bonus equal to the wave number
// (GDD §8) and starts a short intermission before the next wave.
class WaveDirector {
public:
    WaveDirector(Grid& grid, entt::registry& registry, GameState& state, const GameConfig& config);

    void update(float dtSeconds);

    // How many enemies of the current/next wave have yet to spawn -- used by the HUD.
    int enemiesRemainingToSpawn() const { return currentWaveEnemyCount_ - enemiesSpawnedThisWave_; }
    float intermissionSecondsRemaining() const { return intermissionTimer_; }

private:
    enum class Phase { Intermission, Spawning, WaitingForClear };

    Grid& grid_;
    entt::registry& registry_;
    GameState& state_;
    const GameConfig& config_;

    Phase phase_ = Phase::Intermission;
    int baseCountForCurrentWave_ = 0;
    int currentWaveEnemyCount_ = 0;
    int enemiesSpawnedThisWave_ = 0;
    float spawnTimer_ = 0.0f;
    float intermissionTimer_ = 1.0f;

    void startWave();
    void spawnEnemy();
    float effectiveSpawnInterval() const;
    int aliveEnemyCount() const;
};

} // namespace td
