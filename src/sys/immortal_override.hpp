//
//  immortal_override.hpp
//  EnTT Pacman
//

#ifndef SYS_IMMORTAL_OVERRIDE_HPP
#define SYS_IMMORTAL_OVERRIDE_HPP

#include <entt/entity/fwd.hpp>

// When any player has ImmortalMode, redirects chase/scatter ghosts to their
// scatter corner so they walk away. Eaten and scared ghosts are left alone:
// eaten ghosts must still reach the house; scared ghosts already wander.
//
// Must run AFTER all setXTarget functions and BEFORE pursueTarget.
void immortalOverride(entt::registry &);

#endif
