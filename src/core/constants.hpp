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

// Settings passed to Mix_OpenAudio when the audio device is opened
constexpr int audioFrequency = 44100;
constexpr int audioOutputChannels = 2;
constexpr int audioChunkSize = 2048;

// Chebyshev radius (in tiles) around the player that reveals from fog of war.
constexpr int fogRevealRadius = 2;
// Each maze tile is split into fogSubdiv x fogSubdiv fog cells. >1 makes the
// reveal edge finer (smoother) than the maze tile grid.
constexpr int fogSubdiv = 2;
// Pixel size of one fog cell. With tileSize=8 and fogSubdiv=2 this is 4.
constexpr int fogCellSize = tileSize / fogSubdiv;
// Tile-row y of the tunnel wrap-around. The literal already appears in
// sys/movement.cpp and sys/can_move.cpp; reveal_fog wraps on the same row.
constexpr int tunnelRow = 10;

#endif
