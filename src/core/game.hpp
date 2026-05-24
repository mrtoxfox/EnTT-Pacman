//
//  game.hpp
//  EnTT Pacman
//
//  Created by Indiana Kernick on 22/9/18.
//  Copyright © 2018 Indiana Kernick. All rights reserved.
//

#ifndef CORE_GAME_HPP
#define CORE_GAME_HPP

#include <random>
#include "maze.hpp"
#include <SDL_render.h>
#include <SDL_scancode.h>
#include "util/sdl_quad_writer.hpp"
#include <entt/entity/registry.hpp>

class Audio;

class Game {
public:
  void init(Audio &);
  // Returns false when the user has asked to quit (ESC).
  bool input(Audio &, SDL_Scancode);
  bool logic(Audio &);
  void render(SDL_Renderer *, SDL::QuadWriter &, int);

private:
  enum class State {
    playing,
    paused,       // SPACE: freeze + dim overlay + "PAUSED" text
    pausedDebug,  // P: freeze only, no overlay (lets you inspect the frame)
    won,
    lost
  };

  entt::registry reg;
  MazeState maze;
  int dots = 0;
  std::mt19937 rand;
  State state = State::playing;
  int ticks = 0;
  bool scattering = true;
  // Sub-tile frame seen on the last `playing` render. Reused while paused so
  // the freeze keeps the exact pixel position the player saw at press time
  // (snapping to 0 would jump the sprite back to the tile boundary).
  int frozenFrame = 0;
};

#endif
