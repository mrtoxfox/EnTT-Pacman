//
//  ghost_speed_effect.hpp
//  EnTT Pacman
//

#ifndef COMP_GHOST_SPEED_EFFECT_HPP
#define COMP_GHOST_SPEED_EFFECT_HPP

// Tag-with-timer components for the three bonus effects. Mutually exclusive
// per ghost: applying one removes the others. The timer counts down logic
// ticks; ghostSpeedTimeout removes the tag at zero and emits a bonusExpired
// SoundEvent. Same shape as ScaredMode / ImmortalMode.

struct FrozenEffect { int timer; };
struct SlowEffect   { int timer; };
struct SpeedEffect  { int timer; };

#endif
