//
//  eat_bonus.hpp
//  EnTT Pacman
//

#ifndef SYS_EAT_BONUS_HPP
#define SYS_EAT_BONUS_HPP

#include <entt/entity/fwd.hpp>

// Detects player-bonus tile collisions. On a hit: removes any prior speed
// effect from every ghost, emplaces the matching effect with a fresh timer,
// emits a bonusApplied SoundEvent, and destroys the bonus entity.
void eatBonus(entt::registry &);

#endif
