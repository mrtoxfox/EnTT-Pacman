//
//  flashlight_render.hpp
//  EnTT Pacman
//

#ifndef SYS_FLASHLIGHT_RENDER_HPP
#define SYS_FLASHLIGHT_RENDER_HPP

#include "core/maze.hpp"
#include <SDL_render.h>
#include <entt/entity/fwd.hpp>

// Draws a flashlight darkness overlay over the playfield.
//
// Picks the single entity with Flashlight + Position + ActualDir + DesiredDir
// (the player). Builds two visibility-polygon triangle fans (forward + back)
// from the player's interpolated apex pixel, then composites a black overlay
// with cone-shaped holes punched out.
//
// `overlay` must be an SDL_TEXTUREACCESS_TARGET texture sized to the playfield
// (tilesPx.x by tilesPx.y) with blend mode SDL_BLENDMODE_BLEND.
// `frame` is the sub-tile pixel offset (frame % tileSize) so the apex tracks
// the sprite's interpolated position smoothly.
void flashlightRender(
  entt::registry &,
  SDL_Renderer *,
  SDL_Texture *overlay,
  const MazeState &,
  int frame
);

#endif
