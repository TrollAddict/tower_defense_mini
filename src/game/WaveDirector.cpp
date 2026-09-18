#include "game/WaveDirector.hpp"

#include <algorithm>
#include <vector>

#include "core/WaveEscalation.hpp"
#include "ecs/Components.hpp"

namespace td {

namespace {
constexpr TintColor kEnemyColor{210, 60, 60};
constexpr float kNotificationSeconds = 2.5f;
}

WaveDirector::WaveDirector(Grid& grid, entt::registry& registry, GameState& state, const GameConfig& config)
    : grid_(grid),
      registry_(registry),
      state_(state),
      config_(config),
      intermissionTimer_(config.waves.initialBuildPhaseSeconds) {}

void WaveDirector::skipIntermission() {
    if (phase_ == Phase::Intermission) {
        intermissionTimer_ = 0.0f;
    }
}

int WaveDirector::giveUp() {
    if (phase_ == Phase::Intermission) {
        return 0; // nothing active to give up on
    }

    enemiesSpawnedThisWave_ = currentWaveEnemyCount_; // cancel any of this wave's unspawned enemies

    std::vector<entt::entity> stragglers(registry_.view<EnemyTag>().begin(), registry_.view<EnemyTag>().end());
    for (auto entity : stragglers) {
        state_.damageCastle(config_.enemy.castleDamage);
        registry_.destroy(entity);
    }

    state_.addCurrency(state_.waveNumber()); // same wave-clear bonus as a natural clear (GDD §8)
    intermissionTimer_ = config_.waves.timeBetweenWavesSeconds;
    phase_ = Phase::Intermission;

    const int converted = static_cast<int>(stragglers.size());
    setNotification("Gave up on wave " + std::to_string(state_.waveNumber()) + " -- " + std::to_string(converted) +
                     (converted == 1 ? " enemy hit the castle" : " enemies hit the castle"));
    return converted;
}

void WaveDirector::update(float dtSeconds) {
    if (notificationTimer_ > 0.0f) {
        notificationTimer_ -= dtSeconds;
    }

    switch (phase_) {
        case Phase::Intermission:
            intermissionTimer_ -= dtSeconds;
            if (intermissionTimer_ <= 0.0f) {
                startWave();
            }
            break;

        case Phase::Spawning:
            spawnTimer_ -= dtSeconds;
            if (spawnTimer_ <= 0.0f && enemiesSpawnedThisWave_ < currentWaveEnemyCount_) {
                spawnEnemy();
                ++enemiesSpawnedThisWave_;
                spawnTimer_ = effectiveSpawnInterval();
            }
            if (enemiesSpawnedThisWave_ >= currentWaveEnemyCount_) {
                phase_ = Phase::WaitingForClear;
            }
            break;

        case Phase::WaitingForClear:
            if (aliveEnemyCount() == 0) {
                state_.addCurrency(state_.waveNumber()); // wave-clear bonus == wave number (GDD §8)
                intermissionTimer_ = config_.waves.timeBetweenWavesSeconds;
                phase_ = Phase::Intermission;
            }
            break;
    }
}

void WaveDirector::startWave() {
    const int wave = state_.waveNumber() + 1;
    state_.setWaveNumber(wave);

    if (wave == 1) {
        baseCountForCurrentWave_ = config_.waves.baseEnemyCount;
    } else {
        baseCountForCurrentWave_ = nextWaveEnemyCount(baseCountForCurrentWave_, config_.waves.growthRatePerWave);
    }
    currentWaveEnemyCount_ = baseCountForCurrentWave_ * std::max(1, state_.difficulty().waveSizeMultiplier);

    enemiesSpawnedThisWave_ = 0;
    spawnTimer_ = 0.0f; // spawn the first enemy of the wave immediately
    phase_ = Phase::Spawning;
}

void WaveDirector::spawnEnemy() {
    const CellCoord& spawn = grid_.spawnCell();
    const float escalatedHealth =
        escalatedEnemyHealth(config_.enemy.health, config_.waves.enemyHealthGrowthRatePerWave, state_.waveNumber());
    const float health = escalatedHealth * static_cast<float>(std::max(1, state_.difficulty().enemyHealthMultiplier));
    const float speed = escalatedEnemySpeed(config_.enemy.speedTilesPerSecond,
                                             config_.waves.enemySpeedGrowthRatePer5Waves, state_.waveNumber());

    entt::entity entity = registry_.create();
    registry_.emplace<Position>(entity, spawn.x + 0.5f, spawn.y + 0.5f);
    registry_.emplace<Health>(entity, health, health);
    registry_.emplace<Speed>(entity, speed);
    registry_.emplace<EnemyTag>(entity);
    registry_.emplace<TintColor>(entity, kEnemyColor);
}

float WaveDirector::effectiveSpawnInterval() const {
    const int multiplier = std::max(1, state_.difficulty().spawnIntervalMultiplier);
    return config_.waves.spawnIntervalSeconds / static_cast<float>(multiplier);
}

int WaveDirector::aliveEnemyCount() const {
    return static_cast<int>(registry_.view<EnemyTag>().size());
}

void WaveDirector::setNotification(const std::string& message) {
    notification_ = message;
    notificationTimer_ = kNotificationSeconds;
}

} // namespace td
