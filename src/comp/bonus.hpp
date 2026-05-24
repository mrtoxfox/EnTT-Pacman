//
//  bonus.hpp
//  EnTT Pacman
//

#ifndef COMP_BONUS_HPP
#define COMP_BONUS_HPP

#include <cstdint>

// The kind of effect a bonus applies on pickup. Mapped 1:1 to the three
// ghost-speed effect tags (Frozen / Slow / Speed).
enum class BonusKind : std::uint8_t {
  freeze,
  slow,
  speed
};

// One pickup currently sitting on the maze. The entity also carries Position.
// lifeTimer counts down to despawn if the player doesn't reach it; the bonus
// disappears silently on zero.
struct Bonus {
  BonusKind kind;
  int lifeTimer;
};

#endif
