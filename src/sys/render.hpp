//
//  render.hpp
//  EnTT Pacman
//
//  Created by Indiana Kernick on 24/9/18.
//  Copyright © 2018 Indiana Kernick. All rights reserved.
//

#ifndef SYS_RENDER_HPP
#define SYS_RENDER_HPP

#include <cstdint>
#include "core/maze.hpp"
#include <SDL_render.h>
#include "util/sprites.hpp"
#include <entt/entity/fwd.hpp>
#include "util/sdl_quad_writer.hpp"

// Render the player (the yellow guy)
void playerRender(entt::registry &, SDL::QuadWriter &, int);

// Render the ghosts. Ghosts on unrevealed tiles are skipped.
void ghostRender(entt::registry &, SDL::QuadWriter &, const Grid<std::uint8_t> &fog, int);

// Render the dots and energizers
void dotRender(SDL::QuadWriter &, const MazeState &);

// Fill every unrevealed fog cell with an opaque black fogCellSize x fogCellSize
// rect. Call after the world and ghosts, before the pause overlay.
void fogRender(SDL_Renderer *, const Grid<std::uint8_t> &fog);

// Render a sprite that covers the whole screen (maze, win, lose)
void fullRender(SDL::QuadWriter &, animera::SpriteID);

// Draws a translucent black rectangle over the whole logical viewport.
// Used as the pause-screen visual on top of the frozen scene.
void pauseOverlayRender(SDL_Renderer *);

// Draws "PAUSED" centered on the maze area, on top of the pause overlay.
void pauseTextRender(SDL_Renderer *);

// Draws the HUD strip below the maze: pacman icons for remaining lives on the
// left, current score as bitmap digits on the right.
void hudRender(SDL_Renderer *, SDL::QuadWriter &, entt::registry &);

// Draws an end-of-game summary (spent lives row + score) centered on the maze
// area, under the "you win" / "you lose" text baked into the end-screen sprite.
void summaryRender(SDL_Renderer *, SDL::QuadWriter &, entt::registry &);

#endif
