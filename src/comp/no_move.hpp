//
//  no_move.hpp
//  EnTT Pacman
//

#ifndef COMP_NO_MOVE_HPP
#define COMP_NO_MOVE_HPP

// Per-tick gate consumed by the movement system: entities carrying this tag
// are skipped this tick. Re-issued from scratch each tick by
// applyGhostSpeedGate, so the tag's lifetime is fully owned by that system.
struct NoMove {};

#endif
