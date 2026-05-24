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
#include <SDL_scancode.h>
#include "util/sdl_quad_writer.hpp"
#include <entt/entity/registry.hpp>

class Audio;

class Game {
public:
  void init(Audio &);
  void input(SDL_Scancode);
  bool logic(Audio &);
  void render(SDL::QuadWriter &, int);

private:
  enum class State {
    playing,
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
};

#endif
