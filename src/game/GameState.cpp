#include "game/GameState.hpp"

#include <algorithm>

namespace td {

GameState::GameState(const GameConfig& config)
    : currency_(config.economy.startingCurrency),
      castleHealth_(kCastleMaxHealth),
      difficulty_(config.difficulty) {}

bool GameState::damageCastle(float amount) {
    castleHealth_ = std::max(0.0f, castleHealth_ - amount);
    return castleHealth_ <= 0.0f;
}

} // namespace td
