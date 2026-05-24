//
//  ghost_speed_timeout.hpp
//  EnTT Pacman
//

#ifndef SYS_GHOST_SPEED_TIMEOUT_HPP
#define SYS_GHOST_SPEED_TIMEOUT_HPP

#include <entt/entity/fwd.hpp>

// Decrements the timers on FrozenEffect / SlowEffect / SpeedEffect, removes
// the tag on zero, and emits a single bonusExpired SoundEvent if any effect
// ended this tick. Mirrors ghostScaredTimeout.
void ghostSpeedTimeout(entt::registry &);

#endif
