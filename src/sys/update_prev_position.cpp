//
//  update_prev_position.cpp
//  EnTT Pacman
//

#include "update_prev_position.hpp"

#include "comp/position.hpp"
#include "comp/prev_position.hpp"
#include <entt/entity/registry.hpp>

void updatePrevPosition(entt::registry &reg) {
  auto view = reg.view<Position, PrevPosition>();
  for (const entt::entity e : view) {
    view.get<PrevPosition>(e).p = view.get<Position>(e).p;
  }
}
