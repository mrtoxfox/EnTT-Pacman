//
//  constants.hpp
//  EnTT Pacman
//
//  Created by Indiana Kernick on 22/9/18.
//  Copyright © 2018 Indiana Kernick. All rights reserved.
//

#ifndef CORE_CONSTANTS_HPP
#define CORE_CONSTANTS_HPP

#include "util/pos.hpp"
#include "util/dir.hpp"
#include "util/sprites.hpp"

// The width and height of the maze in tiles
constexpr Pos tiles = {19, 22};
// The pixel size of tiles
constexpr int tileSize = 8;
// The width and height of the maze in pixels
constexpr Pos tilesPx = tiles * tileSize;
// Height in pixels of the HUD strip drawn below the maze (lives + score).
constexpr int hudHeight = tileSize;
// The full logical canvas: maze on top, HUD strip below.
constexpr Pos canvasPx = {tilesPx.x, tilesPx.y + hudHeight};
// The amount of ticks that ghosts are scared for
constexpr int ghostScaredTime = 40;

// Number of lives the player starts with. Game ends when all are lost.
constexpr int startingLives = 3;
// Sprite used to represent a life in the HUD strip.
constexpr animera::SpriteID lifeIconSprite = animera::SpriteID::pacman_2;

// The amount of ticks left on the scared timer before scared ghosts
// start flashing
constexpr int ghostScaredFlashTime = 10;
// Related to flash speed. Higher is slower
constexpr int ghostScaredFlashRate = 4;

// The total number of dots that the player must eat to win the game
constexpr int dotsInMaze = 152;

// Point values
constexpr int dotPoints = 10;
constexpr int energizerPoints = 50;
constexpr int ghostPoints = 200;

// Position where the player spawns
constexpr Pos playerSpawnPos = {9, 16};
// Direction that the player is facing and moving when they spawn
constexpr Dir playerSpawnDir = Dir::left;
// A position just outside the ghost house above the door
constexpr Pos outsideHouse  = { 9,  8};

// Home positions are the places in the house where ghosts go
constexpr Pos blinkyHome    = { 9, 10};
constexpr Pos pinkyHome     = { 9, 10};
constexpr Pos inkyHome      = { 8, 10};
constexpr Pos clydeHome     = {10, 10};
// Scatter positions are targets that ghosts move towards (but never reach)
// when in scatter mode
constexpr Pos blinkyScatter = {18,  0};
constexpr Pos pinkyScatter  = { 0,  0};
constexpr Pos inkyScatter   = {18, 21};
constexpr Pos clydeScatter  = { 0, 21};

// The amount of time ghosts will be in scatter mode before switching
// to chase mode
constexpr int scatterTicks = 15;
// The amount of time ghosts will be in chase mode before switching
// to scatter mode
constexpr int chaseTicks = 40;
// The frame rate. Game logic runs once every tileSize frames, so raising the
// frame rate speeds up the whole game. Bumped from 20 to 30 for a faster pace.
constexpr int fps = 30;

// How long Pac-Man is immortal after being caught. Expressed in logic ticks.
// 5 real-time seconds = (5 * fps) / tileSize logic ticks.
constexpr int immortalTicks = (5 * fps) / tileSize;

// Alpha (0-255) used by SDL_SetRenderDrawColor for the pause overlay
constexpr int pauseOverlayAlpha = 160;
// Alpha used by SDL_SetTextureAlphaMod while the player has ImmortalMode
constexpr int immortalAlpha = 110;

// Flashlight: forward beam length in tiles.
constexpr int flashlightForwardTiles = 6;
// Flashlight: back beam length in tiles.
constexpr int flashlightBackTiles = 1;
// Flashlight: forward cone half-angle in degrees. Wide enough that the lit
// area opens out to roughly the full corridor; walls clip the cone to the
// corridor width where they are present.
constexpr int flashlightForwardHalfDeg = 32;
// Flashlight: back cone half-angle in degrees. Slightly wider for a small
// stub behind the player.
constexpr int flashlightBackHalfDeg = 35;
// Flashlight: perimeter ray count for the forward cone.
constexpr int flashlightForwardRays = 64;
// Flashlight: perimeter ray count for the back cone.
constexpr int flashlightBackRays = 32;
// Flashlight: radius (pixels) of the always-lit halo around the player.
// Guarantees Pac-Man's sprite is fully covered even when the cones leave a
// perpendicular gap.
constexpr int flashlightHaloRadiusPx = 6;
// Flashlight: perimeter ray count for the halo disc.
constexpr int flashlightHaloRays = 20;
// Pixels each ray that hit a wall is extended past the hit point so the
// wall's inner surface (the side facing the light source) is included in the
// lit polygon. Larger values show more wall material but can leak past corners.
constexpr int flashlightWallEdgePx = 3;
// Color of the unlit overlay. Near-black; the small alpha cut lets the maze
// stay faintly visible outside the beam.
constexpr std::uint8_t flashlightDarknessR = 0;
constexpr std::uint8_t flashlightDarknessG = 0;
constexpr std::uint8_t flashlightDarknessB = 0;
// Alpha (0-255) of the unlit overlay. 235 = mostly opaque, with ~8% of the
// underlying maze bleeding through as a dim outline.
constexpr std::uint8_t flashlightDarknessAlpha = 235;
// Light-yellow tint applied additively over the cone area after the darkness
// overlay, so the beam reads as warm light instead of just an absence of dark.
// Values are added to the underlying maze RGB - keep moderate so colors do
// not clip to white.
constexpr std::uint8_t flashlightBeamR = 90;
constexpr std::uint8_t flashlightBeamG = 75;
constexpr std::uint8_t flashlightBeamB = 20;

// Settings passed to Mix_OpenAudio when the audio device is opened
constexpr int audioFrequency = 44100;
constexpr int audioOutputChannels = 2;
constexpr int audioChunkSize = 2048;

#endif
