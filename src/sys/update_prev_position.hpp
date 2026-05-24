//
//  update_prev_position.hpp
//  EnTT Pacman
//

#ifndef SYS_UPDATE_PREV_POSITION_HPP
#define SYS_UPDATE_PREV_POSITION_HPP

#include <entt/entity/fwd.hpp>

// Copies Position into PrevPosition for every entity that has both. Must be
// the first call in Game::logic so render can interpolate from start-of-tick
// to end-of-tick position regardless of how movement and speed effects
// change Position during the tick.
void updatePrevPosition(entt::registry &);

#endif
