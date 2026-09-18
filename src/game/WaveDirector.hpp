#pragma once

#include <string>

#include <entt/entt.hpp>

#include "core/Config.hpp"
#include "core/Grid.hpp"
#include "game/GameState.hpp"

namespace td {

// Drives endless wave spawning (GDD §3/§5/§14). The run opens with a longer
// intermission (initial_build_phase_seconds) so the player can start shaping the maze
// before anything is incoming; every intermission after that (including this first
// one) can be skipped early with Enter. Wave N's enemy count grows 10% off wave
// N-1's, rounded down (GDD §5's escalation curve), then scaled by the player's
// difficulty multiplier. A wave is "cleared" when every enemy from it has either died
// or reached the castle; clearing awards a currency bonus equal to the wave number
// (GDD §8) and starts a short intermission before the next wave.
class WaveDirector {
public:
    WaveDirector(Grid& grid, entt::registry& registry, GameState& state, const GameConfig& config);

    void update(float dtSeconds);

    // Ends the current intermission immediately, starting the next wave on the very
    // next update() tick. A no-op outside the Intermission phase.
    void skipIntermission();

    // Fast-forwards a straggler-dominated wave instead of making the player wait for
    // the last few enemies to walk the rest of a (possibly very long) path: every
    // currently-alive enemy from this wave is converted straight to castle damage and
    // despawned, any of this wave's enemies that hadn't spawned yet are cancelled, and
    // the wave is immediately marked cleared (same currency bonus as a natural clear
    // -- the cost of giving up is the castle damage taken, not a forfeited bonus). A
    // no-op during Intermission, since there's no active wave to give up on. Returns
    // how many enemies were converted (0 if it was a no-op).
    int giveUp();

    // How many enemies of the current/next wave have yet to spawn -- used by the HUD.
    int enemiesRemainingToSpawn() const { return currentWaveEnemyCount_ - enemiesSpawnedThisWave_; }
    float intermissionSecondsRemaining() const { return intermissionTimer_; }
    bool isInIntermission() const { return phase_ == Phase::Intermission; }

    const std::string& notification() const { return notification_; }
    bool hasActiveNotification() const { return notificationTimer_ > 0.0f; }

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
    float intermissionTimer_; // set from config.waves.initialBuildPhaseSeconds in the constructor
    std::string notification_;
    float notificationTimer_ = 0.0f;

    void startWave();
    void spawnEnemy();
    float effectiveSpawnInterval() const;
    int aliveEnemyCount() const;
    void setNotification(const std::string& message);
};

} // namespace td
