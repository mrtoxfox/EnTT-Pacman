//
//  bonus_timeout.cpp
//  EnTT Pacman
//

#include "bonus_timeout.hpp"

#include <vector>
#include "comp/bonus.hpp"
#include <entt/entity/registry.hpp>

void bonusTimeout(entt::registry &reg) {
  std::vector<entt::entity> expired;
  auto view = reg.view<Bonus>();
  for (const entt::entity e : view) {
    Bonus &bonus = view.get<Bonus>(e);
    if (--bonus.lifeTimer <= 0) {
      expired.push_back(e);
    }
  }
  for (const entt::entity e : expired) {
    reg.destroy(e);
  }
}
