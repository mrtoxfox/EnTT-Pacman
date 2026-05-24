//
//  apply_ghost_speed.hpp
//  EnTT Pacman
//

#ifndef SYS_APPLY_GHOST_SPEED_HPP
#define SYS_APPLY_GHOST_SPEED_HPP

#include "core/maze.hpp"
#include <entt/entity/fwd.hpp>

// Adds an extra one-tile step for every ghost with SpeedEffect. Call this
// immediately after movement(reg) so it sees the just-applied tile step.
// Implements the "double rate" half of the bonus system.
//
// The extra step uses the ghost's current ActualDir without re-running the
// AI, so a speed-boosted ghost overshoots a junction by one tile before it
// can turn (the turn happens next tick via pursueTarget). canMove still
// blocks running into a wall, but the AI path can look slightly wrong for
// a tick. Accepted trade-off; documented in
// prompts/universal-bonus-system-design.md.
void applyGhostSpeedExtraStep(entt::registry &, const MazeState &);

// Re-issues NoMove tags from scratch for the upcoming tick:
//   - every FrozenEffect ghost gets NoMove,
//   - every SlowEffect ghost gets NoMove on alternating ticks, keyed off the
//     effect's own timer parity (not the global tick),
//   - SpeedEffect ghosts never get NoMove; their extra step is handled above.
// All existing NoMove tags are cleared first, so this system fully owns the
// tag's lifetime. Call at the end of Game::logic, just before audio().
void applyGhostSpeedGate(entt::registry &);

#endif
