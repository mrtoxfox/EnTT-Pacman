//
//  bonus_timeout.hpp
//  EnTT Pacman
//

#ifndef SYS_BONUS_TIMEOUT_HPP
#define SYS_BONUS_TIMEOUT_HPP

#include <entt/entity/fwd.hpp>

// Decrements Bonus::lifeTimer on every bonus entity and destroys those that
// reach zero. Silent: uncollected bonuses despawn without a sound.
void bonusTimeout(entt::registry &);

#endif
