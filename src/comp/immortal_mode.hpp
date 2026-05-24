//
//  immortal_mode.hpp
//  EnTT Pacman
//

#ifndef COMP_IMMORTAL_MODE_HPP
#define COMP_IMMORTAL_MODE_HPP

#include "core/constants.hpp"

// Tag-with-timer for a player who was just caught. While this is present:
//   - playerGhostCollide ignores collisions with chase/scatter ghosts.
//   - The chase/scatter set_target systems are overridden so ghosts walk away.
//   - playerRender draws the player semi-transparent.
// Mirrors ScaredMode in shape and lifecycle.
struct ImmortalMode {
  int timer = immortalTicks;
};

#endif
