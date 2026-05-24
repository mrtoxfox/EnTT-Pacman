//
//  immortal_timeout.hpp
//  EnTT Pacman
//

#ifndef SYS_IMMORTAL_TIMEOUT_HPP
#define SYS_IMMORTAL_TIMEOUT_HPP

#include <entt/entity/fwd.hpp>

// Counts down ImmortalMode::timer on every player. Removes the tag when the
// timer reaches zero, ending the immortality window. Mirrors ghostScaredTimeout.
void immortalTimeout(entt::registry &);

#endif
