//
//  player_input.cpp
//  EnTT Pacman
//
//  Created by Indiana Kernick on 19/9/18.
//  Copyright © 2018 Indiana Kernick. All rights reserved.
//

#include "player_input.hpp"

#include "comp/dir.hpp"
#include "comp/player.hpp"
#include "sys/can_move.hpp"
#include "comp/position.hpp"
#include <entt/entity/registry.hpp>

namespace {

Dir readDir(const SDL_Scancode key) {
  switch (key) {
    case SDL_SCANCODE_W:
    case SDL_SCANCODE_UP:
      return Dir::up;
    case SDL_SCANCODE_D:
    case SDL_SCANCODE_RIGHT:
      return Dir::right;
    case SDL_SCANCODE_S:
    case SDL_SCANCODE_DOWN:
      return Dir::down;
    case SDL_SCANCODE_A:
    case SDL_SCANCODE_LEFT:
      return Dir::left;
    default:
      return Dir::none;
  }
}

}

bool playerInput(entt::registry &reg, const MazeState &maze, const SDL_Scancode key) {
  const Dir dir = readDir(key);
  if (dir == Dir::none) {
    return false;
  }
  auto view = reg.view<Player, Position, ActualDir, DesiredDir>();
  for (const entt::entity e : view) {
    view.get<DesiredDir>(e).d = dir;
    // wallCollide runs after movement, so a DesiredDir change made between
    // ticks only takes effect one tile past the intended junction. Applying
    // ActualDir here when the turn is valid at the current tile means the
    // next movement step honors the input.
    if (canMove(reg, maze, e, view.get<Position>(e).p, dir)) {
      view.get<ActualDir>(e).d = dir;
    }
  }
  return true;
}
