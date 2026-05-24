//
//  immortal_timeout.cpp
//  EnTT Pacman
//

#include "immortal_timeout.hpp"

#include "comp/player.hpp"
#include "comp/immortal_mode.hpp"
#include <entt/entity/registry.hpp>

void immortalTimeout(entt::registry &reg) {
  auto view = reg.view<Player, ImmortalMode>();
  for (const entt::entity e : view) {
    ImmortalMode &immortal = view.get<ImmortalMode>(e);
    --immortal.timer;
    if (immortal.timer <= 0) {
      reg.remove<ImmortalMode>(e);
    }
  }
}
