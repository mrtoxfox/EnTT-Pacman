//
//  bonus_render.cpp
//  EnTT Pacman
//

#include "bonus_render.hpp"

#include "comp/bonus.hpp"
#include "comp/position.hpp"
#include "util/sdl_check.hpp"
#include "core/constants.hpp"
#include <entt/entity/registry.hpp>

namespace {

struct Color { Uint8 r, g, b; };

Color colorFor(const BonusKind kind) {
  switch (kind) {
    case BonusKind::freeze: return {120, 200, 255}; // light cyan
    case BonusKind::slow:   return { 80, 220, 100}; // green
    case BonusKind::speed:  return {255, 110,  60}; // red-orange
  }
  return {255, 255, 255};
}

}

void bonusRender(SDL_Renderer *renderer, entt::registry &reg) {
  constexpr int inset = 2;
  SDL_CHECK(SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE));
  const auto view = reg.view<Bonus, Position>();
  for (const entt::entity e : view) {
    const Pos p = view.get<Position>(e).p;
    const Color c = colorFor(view.get<Bonus>(e).kind);
    SDL_CHECK(SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 255));
    const SDL_Rect rect{
      p.x * tileSize + inset,
      p.y * tileSize + inset,
      tileSize - 2 * inset,
      tileSize - 2 * inset
    };
    SDL_CHECK(SDL_RenderFillRect(renderer, &rect));
  }
}
