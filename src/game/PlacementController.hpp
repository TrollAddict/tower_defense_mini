#pragma once

#include <string>
#include <vector>

#include <entt/entt.hpp>

#include "core/Config.hpp"
#include "core/FlowField.hpp"
#include "core/Grid.hpp"
#include "game/GameState.hpp"

namespace td {

enum class StructureType { Tower, Wall };

// Implements GDD §7's "place-then-verify-then-possibly-revert" rule: a placement is
// applied immediately, the flow field is recomputed, and if the castle becomes
// unreachable from the spawn point that same placement is undone and refunded. Since
// the check runs after every single edit, "the last tower or wall" is always
// unambiguous — there's never more than one candidate to revert.
class PlacementController {
public:
    PlacementController(Grid& grid, FlowField& flowField, entt::registry& registry, GameState& state,
                         const GameConfig& config);

    // Returns true if the structure was placed and kept. False covers three distinct
    // rejection reasons (cell occupied/out of bounds, insufficient currency, or a
    // revert because no path remained) — callers only needing pass/fail can ignore
    // the distinction; UI wanting the reason should read notification() right after.
    bool place(StructureType type, int x, int y);

    // Removing a structure can only ever open the path graph back up (GDD §7), so it
    // never needs a reachability check or revert.
    void remove(int x, int y);

    void update(float dtSeconds);
    const std::string& notification() const { return notification_; }
    bool hasActiveNotification() const { return notificationTimer_ > 0.0f; }

private:
    Grid& grid_;
    FlowField& flowField_;
    entt::registry& registry_;
    GameState& state_;
    const GameConfig& config_;
    std::vector<entt::entity> entityAt_;

    std::string notification_;
    float notificationTimer_ = 0.0f;

    int index(int x, int y) const { return y * grid_.width() + x; }
    void setNotification(const std::string& message);
};

} // namespace td
