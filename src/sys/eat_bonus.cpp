//
//  eat_bonus.cpp
//  EnTT Pacman
//

#include "eat_bonus.hpp"

#include <vector>
#include "comp/bonus.hpp"
#include "comp/ghost.hpp"
#include "comp/player.hpp"
#include "comp/position.hpp"
#include "core/constants.hpp"
#include "comp/sound_event.hpp"
#include "comp/ghost_mode.hpp"
#include "comp/ghost_speed_effect.hpp"
#include <entt/entity/registry.hpp>

namespace {

void applyToGhosts(entt::registry &reg, const BonusKind kind) {
  const auto view = reg.view<Ghost>();
  for (const entt::entity g : view) {
    // Skip eyes (ghost returning home). Mirrors ghostScared in
    // change_ghost_mode.cpp: an eaten ghost is mid-respawn and shouldn't
    // pick up gameplay effects until it leaves the house.
    if (reg.has<EatenMode>(g)) continue;
    reg.remove_if_exists<FrozenEffect, SlowEffect, SpeedEffect>(g);
    switch (kind) {
      case BonusKind::freeze: reg.emplace<FrozenEffect>(g, bonusEffectTicks); break;
      case BonusKind::slow:   reg.emplace<SlowEffect>(g,   bonusEffectTicks); break;
      case BonusKind::speed:  reg.emplace<SpeedEffect>(g,  bonusEffectTicks); break;
    }
  }
}

}

void eatBonus(entt::registry &reg) {
  // Collect (bonus, kind) first, then mutate. The effect application touches
  // ghost entities outside the player/bonus views, which CLAUDE.md warns is
  // unsafe to do mid-iteration even when the current views don't see them.
  std::vector<std::pair<entt::entity, BonusKind>> consumed;
  {
    const auto players = reg.view<Player, Position>();
    const auto bonuses = reg.view<Bonus, Position>();
    for (const entt::entity p : players) {
      const Pos playerPos = players.get<Position>(p).p;
      for (const entt::entity b : bonuses) {
        if (bonuses.get<Position>(b).p != playerPos) continue;
        consumed.push_back({b, bonuses.get<Bonus>(b).kind});
      }
    }
  }
  for (const auto &[bonus, kind] : consumed) {
    applyToGhosts(reg, kind);
    reg.emplace<SoundEvent>(reg.create(), SoundId::bonusApplied);
    reg.destroy(bonus);
  }
}
