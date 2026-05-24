//
//  flashlight.hpp
//  EnTT Pacman
//

#ifndef COMP_FLASHLIGHT_HPP
#define COMP_FLASHLIGHT_HPP

// Tag component. Entities with a Flashlight cast a cone of light during
// rendering. The cone's apex follows the entity's Position + sub-tile offset,
// and its direction comes from the entity's DesiredDir.
struct Flashlight {};

#endif
