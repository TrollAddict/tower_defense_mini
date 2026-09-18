#pragma once

#include "core/Config.hpp"

namespace td {

constexpr float kCastleMaxHealth = 50.0f;

// Everything that isn't grid/entity state: currency, castle HP, wave/kill counters,
// and the player-adjustable difficulty multipliers (GDD §14).
class GameState {
public:
    explicit GameState(const GameConfig& config);

    int currency() const { return currency_; }
    bool canAfford(int cost) const { return currency_ >= cost; }
    void spend(int cost) { currency_ -= cost; }
    void refund(int cost) { currency_ += cost; }
    void addCurrency(int amount) { currency_ += amount; }

    float castleHealth() const { return castleHealth_; }
    float castleMaxHealth() const { return kCastleMaxHealth; }
    // Returns true if this damage destroyed the castle.
    bool damageCastle(float amount);
    bool isCastleDestroyed() const { return castleHealth_ <= 0.0f; }

    int waveNumber() const { return waveNumber_; }
    void setWaveNumber(int wave) { waveNumber_ = wave; }

    int kills() const { return kills_; }
    void addKill() { ++kills_; }

    DifficultyConfig& difficulty() { return difficulty_; }
    const DifficultyConfig& difficulty() const { return difficulty_; }

private:
    int currency_;
    float castleHealth_;
    int waveNumber_ = 0;
    int kills_ = 0;
    DifficultyConfig difficulty_;
};

} // namespace td
