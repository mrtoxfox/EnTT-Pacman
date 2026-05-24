//
//  eat_dots.cpp
//  EnTT Pacman
//
//  Created by Indiana Kernick on 22/9/18.
//  Copyright © 2018 Indiana Kernick. All rights reserved.
//

#include "eat_dots.hpp"

#include "comp/score.hpp"
#include "comp/player.hpp"
#include "comp/position.hpp"
#include "core/constants.hpp"
#include "comp/sound_event.hpp"
#include <entt/entity/registry.hpp>

namespace {

int countConsumptions(entt::registry &reg, MazeState &maze, const Tile food) {
  int count = 0;
  const auto view = reg.view<Player, Position>();
  for (const entt::entity e : view) {
    const Pos pos = view.get<Position>(e).p;
    if (maze.outOfRange(pos)) {
      continue;
    }
    Tile &tile = maze[pos];
    if (tile == food) {
      ++count;
      tile = Tile::empty;
    }
  }
  return count;
}

}

int eatDots(entt::registry &reg, MazeState &maze) {
  const int count = countConsumptions(reg, maze, Tile::dot);
  if (count > 0) {
    reg.emplace<SoundEvent>(reg.create(), SoundId::chomp);
    const auto view = reg.view<Player, Score>();
    for (const entt::entity e : view) {
      view.get<Score>(e).value += count * dotPoints;
    }
  }
  return count;
}

bool eatEnergizer(entt::registry &reg, MazeState &maze) {
  const int count = countConsumptions(reg, maze, Tile::energizer);
  if (count > 0) {
    reg.emplace<SoundEvent>(reg.create(), SoundId::energizer);
    const auto view = reg.view<Player, Score>();
    for (const entt::entity e : view) {
      view.get<Score>(e).value += count * energizerPoints;
    }
  }
  return count > 0;
}
