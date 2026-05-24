//
//  prev_position.hpp
//  EnTT Pacman
//

#ifndef COMP_PREV_POSITION_HPP
#define COMP_PREV_POSITION_HPP

#include "util/pos.hpp"

// The entity's tile position at the start of the current logic tick.
// updatePrevPosition copies Position into here before movement runs; the
// render systems interpolate from PrevPosition to Position over the tileSize
// frames before the next tick. This is what makes Speed ghosts (Position
// jumps two tiles per tick) slide smoothly and what stops Frozen / Slow
// ghosts from snapping back when their Position doesn't change.
struct PrevPosition {
  Pos p;
};

#endif
