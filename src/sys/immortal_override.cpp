//
//  immortal_override.cpp
//  EnTT Pacman
//

#include "immortal_override.hpp"

#include "comp/ghost.hpp"
#include "comp/player.hpp"
#include "comp/target.hpp"
#include "comp/ghost_mode.hpp"
#include "comp/immortal_mode.hpp"
#include "comp/home_position.hpp"
#include <entt/entity/registry.hpp>

void immortalOverride(entt::registry &reg) {
  // Cheap presence check: anything in the ImmortalMode view means override.
  if (reg.view<Player, ImmortalMode>().empty()) {
    return;
  }
  auto view = reg.view<Ghost, Target, HomePosition>();
  for (const entt::entity e : view) {
    if (reg.has<EatenMode>(e) || reg.has<ScaredMode>(e)) {
      continue;
    }
    view.get<Target>(e).p = view.get<HomePosition>(e).scatter;
  }
}
