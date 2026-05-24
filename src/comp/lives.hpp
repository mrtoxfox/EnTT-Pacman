//
//  lives.hpp
//  EnTT Pacman
//

#ifndef COMP_LIVES_HPP
#define COMP_LIVES_HPP

#include "core/constants.hpp"

// Lives remaining for a player. Decremented when a ghost catches a non-immortal
// Pac-Man. When this hits zero, the game ends.
struct Lives {
  int remaining = startingLives;
};

#endif
