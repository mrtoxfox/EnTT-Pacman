//
//  ghost_speed_timeout.cpp
//  EnTT Pacman
//

#include "ghost_speed_timeout.hpp"

#include "comp/sound_event.hpp"
#include "comp/ghost_speed_effect.hpp"
#include <entt/entity/registry.hpp>

namespace {

template <typename Effect>
bool tickEffect(entt::registry &reg) {
  bool expired = false;
  auto view = reg.view<Effect>();
  for (const entt::entity e : view) {
    Effect &eff = view.template get<Effect>(e);
    --eff.timer;
    if (eff.timer <= 0) {
      reg.remove<Effect>(e);
      expired = true;
    }
  }
  return expired;
}

}

void ghostSpeedTimeout(entt::registry &reg) {
  bool any = false;
  any = tickEffect<FrozenEffect>(reg) || any;
  any = tickEffect<SlowEffect>(reg)   || any;
  any = tickEffect<SpeedEffect>(reg)  || any;
  // One sound per tick regardless of how many ghosts lost an effect: avoids a
  // four-stack of identical SFX when all ghosts share the same timer.
  if (any) {
    reg.emplace<SoundEvent>(reg.create(), SoundId::bonusExpired);
  }
}
