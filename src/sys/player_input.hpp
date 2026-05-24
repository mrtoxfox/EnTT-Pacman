//
//  player_input.hpp
//  EnTT Pacman
//
//  Created by Indiana Kernick on 19/9/18.
//  Copyright © 2018 Indiana Kernick. All rights reserved.
//

#ifndef SYS_PLAYER_INPUT_HPP
#define SYS_PLAYER_INPUT_HPP

#include "core/maze.hpp"
#include <SDL_scancode.h>
#include <entt/entity/fwd.hpp>

// Functions that read input should return whether they consumed the input.
// If an input function hasn't consumed an input, try the next one.
// Since there is only one input function in this game, we don't really
// need this but I thought I'd show it anyway.

// Sets the player's DesiredDir from the pressed key, and also updates
// ActualDir immediately when the new direction is walkable from the player's
// current tile. The immediate ActualDir update closes a one-tick lag: without
// it, a key press arriving mid-interpolation only takes effect after the next
// movement tick has already advanced past the junction.

bool playerInput(entt::registry &, const MazeState &, SDL_Scancode);

#endif
