//
//  reveal_fog.hpp
//  EnTT Pacman
//

#ifndef SYS_REVEAL_FOG_HPP
#define SYS_REVEAL_FOG_HPP

#include <cstdint>
#include "util/grid.hpp"
#include <entt/entity/fwd.hpp>

// Marks tiles within fogRevealRadius (Chebyshev) of the player as revealed.
// Wraps the reveal across the tunnel row (y == tunnelRow). 0 = unrevealed,
// 1 = revealed.
void revealFog(entt::registry &, Grid<std::uint8_t> &fog);

#endif
