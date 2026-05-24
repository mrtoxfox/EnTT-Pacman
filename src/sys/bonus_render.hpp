//
//  bonus_render.hpp
//  EnTT Pacman
//

#ifndef SYS_BONUS_RENDER_HPP
#define SYS_BONUS_RENDER_HPP

#include <SDL_render.h>
#include <entt/entity/fwd.hpp>

// Draws every Bonus entity as a colored square inset inside its tile, using
// SDL_RenderFillRect. Bypasses the Animera-generated sprite sheet (same
// approach as the HUD digits and the pause overlay).
void bonusRender(SDL_Renderer *, entt::registry &);

#endif
