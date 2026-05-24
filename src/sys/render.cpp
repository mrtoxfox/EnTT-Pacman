//
//  render.cpp
//  EnTT Pacman
//
//  Created by Indiana Kernick on 24/9/18.
//  Copyright © 2018 Indiana Kernick. All rights reserved.
//

#include "render.hpp"

#include <vector>
#include <cstdint>
#include "comp/dir.hpp"
#include "comp/score.hpp"
#include "comp/lives.hpp"
#include "comp/player.hpp"
#include "comp/sprite.hpp"
#include "comp/position.hpp"
#include "util/sdl_check.hpp"
#include "core/constants.hpp"
#include "comp/ghost_mode.hpp"
#include "util/dir_to_pos.hpp"
#include "comp/immortal_mode.hpp"
#include <entt/entity/registry.hpp>

void playerRender(entt::registry &reg, SDL::QuadWriter &writer, const int frame) {
  const auto view = reg.view<Position, ActualDir, DesiredDir, PlayerSprite>();
  for (const entt::entity e : view) {
    const Pos pos = view.get<Position>(e).p * tileSize;
    const Dir actualDir = view.get<ActualDir>(e).d;
    const double angle = static_cast<double>(view.get<DesiredDir>(e).d) * 90.0;
    const bool immortal = reg.has<ImmortalMode>(e);
    if (immortal) {
      writer.setAlphaMod(immortalAlpha);
    }
    writer.tilePos(pos + toPos(actualDir, frame), Pos{tileSize, tileSize}, angle);
    writer.tileTex(view.get<PlayerSprite>(e).id + frame);
    writer.render();
    if (immortal) {
      writer.setAlphaMod(255);
    }
  }
}

void ghostRender(entt::registry &reg, SDL::QuadWriter &writer, const Grid<std::uint8_t> &fog, const int frame) {
  const auto view = reg.view<Position, ActualDir, GhostSprite>();
  for (const entt::entity e : view) {
    const Pos tilePos = view.get<Position>(e).p;
    // Off-grid tile positions occur for one tick on the tunnel row during wrap
    // (x = -1 or tiles.x). Draw those ghosts: the player's reveal on tunnelRow
    // already covers both mouths.
    // Fog is at fogSubdiv x fogSubdiv resolution per tile, so check the cells
    // the ghost actually occupies. Visible if any one of them is revealed.
    const Pos cell0 = tilePos * fogSubdiv;
    bool anyRevealed = false;
    for (int cy = 0; cy != fogSubdiv && !anyRevealed; ++cy) {
      for (int cx = 0; cx != fogSubdiv && !anyRevealed; ++cx) {
        const Pos c{cell0.x + cx, cell0.y + cy};
        if (fog.outOfRange(c) || fog[c]) {
          anyRevealed = true;
        }
      }
    }
    if (!anyRevealed) continue;
    const Pos pos = tilePos * tileSize;
    const Dir actualDir = view.get<ActualDir>(e).d;
    writer.tilePos(pos + toPos(actualDir, frame), Pos{tileSize, tileSize});
    const int dirOffset = (
      actualDir == Dir::none ? 0 : static_cast<int>(actualDir)
    );
    const GhostSprite sprite = view.get<GhostSprite>(e);
    if (reg.has<ChaseMode>(e) || reg.has<ScatterMode>(e)) {
      writer.tileTex(sprite.id + dirOffset);
    } else if (reg.has<ScaredMode>(e)) {
      const int scaredTimer = reg.get<ScaredMode>(e).timer;
      const int flash = (
        scaredTimer <= ghostScaredFlashTime ? (frame / ghostScaredFlashRate) % 2 : 0
      );
      writer.tileTex(animera::SpriteID::scared_beg_ + flash);
    } else if (reg.has<EatenMode>(e)) {
      writer.tileTex(animera::SpriteID::eyes_beg_ + dirOffset);
    }
    writer.render();
  }
}

void dotRender(SDL::QuadWriter &writer, const MazeState &maze) {
  for (int y = 0; y != maze.height(); ++y) {
    for (int x = 0; x != maze.width(); ++x) {
      const Tile tile = maze[{x, y}];
      writer.tilePos({x * tileSize, y * tileSize}, {tileSize, tileSize});
      if (tile == Tile::dot) {
        writer.tileTex(animera::SpriteID::dot);
      } else if (tile == Tile::energizer) {
        writer.tileTex(animera::SpriteID::energizer);
      } else {
        continue;
      }
      writer.render();
    }
  }
}

void fullRender(SDL::QuadWriter &writer, const animera::SpriteID sprite) {
  writer.tilePos({0, 0}, tilesPx);
  writer.tileTex(sprite);
  writer.render();
}

void fogRender(SDL_Renderer *renderer, const Grid<std::uint8_t> &fog) {
  std::vector<SDL_Rect> rects;
  rects.reserve(fog.width() * fog.height());
  for (int y = 0; y != fog.height(); ++y) {
    for (int x = 0; x != fog.width(); ++x) {
      if (!fog[{x, y}]) {
        rects.push_back({x * fogCellSize, y * fogCellSize, fogCellSize, fogCellSize});
      }
    }
  }
  if (rects.empty()) return;
  SDL_CHECK(SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE));
  SDL_CHECK(SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255));
  SDL_CHECK(SDL_RenderFillRects(renderer, rects.data(), static_cast<int>(rects.size())));
}

void pauseOverlayRender(SDL_Renderer *renderer) {
  SDL_CHECK(SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND));
  SDL_CHECK(SDL_SetRenderDrawColor(renderer, 0, 0, 0, pauseOverlayAlpha));
  const SDL_Rect full{0, 0, tilesPx.x, tilesPx.y};
  SDL_CHECK(SDL_RenderFillRect(renderer, &full));
  SDL_CHECK(SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE));
}

namespace {

// 3x5 bitmap font. Row-major, MSB = top-left pixel.
// 15 bits used per glyph; bit (14 - (row*3 + col)) is the pixel.
constexpr int glyphW = 3;
constexpr int glyphH = 5;
constexpr int glyphSpacing = 1;

constexpr std::uint16_t digitBits[10] = {
  0b111'101'101'101'111, // 0
  0b010'110'010'010'111, // 1
  0b111'001'111'100'111, // 2
  0b111'001'111'001'111, // 3
  0b101'101'111'001'001, // 4
  0b111'100'111'001'111, // 5
  0b111'100'111'101'111, // 6
  0b111'001'001'001'001, // 7
  0b111'101'111'101'111, // 8
  0b111'101'111'001'111, // 9
};

// Letters used by pauseTextRender. Indices match "PAUSED".
constexpr std::uint16_t pausedLetterBits[6] = {
  0b111'101'111'100'100, // P
  0b010'101'111'101'101, // A
  0b101'101'101'101'111, // U
  0b111'100'111'001'111, // S
  0b111'100'111'100'111, // E
  0b110'101'101'101'110, // D
};

// Draws a 3x5 glyph scaled by `scale`. Each set bit becomes a `scale`x`scale`
// rectangle. The caller sets the draw color.
void drawGlyph(SDL_Renderer *renderer, std::uint16_t bits, int x, int y, int scale) {
  for (int row = 0; row != glyphH; ++row) {
    for (int col = 0; col != glyphW; ++col) {
      const int shift = 14 - (row * glyphW + col);
      if ((bits >> shift) & 1) {
        const SDL_Rect px{x + col * scale, y + row * scale, scale, scale};
        SDL_CHECK(SDL_RenderFillRect(renderer, &px));
      }
    }
  }
}

void drawDigit(SDL_Renderer *renderer, const int digit, const int x, const int y) {
  drawGlyph(renderer, digitBits[digit], x, y, 1);
}

// Splits `value` into decimal digits (least significant first) and pads up to
// `minDigits` zeroes. Returns the number of digits written.
int explodeDigits(int value, int *out, const int minDigits) {
  int count = 0;
  do {
    out[count++] = value % 10;
    value /= 10;
  } while (value > 0);
  while (count < minDigits) {
    out[count++] = 0;
  }
  return count;
}

int numberWidth(const int count) {
  return count * glyphW + (count - 1) * glyphSpacing;
}

void drawNumber(SDL_Renderer *renderer, const int *digits, const int count, int x, const int y) {
  for (int i = count - 1; i >= 0; --i) {
    drawDigit(renderer, digits[i], x, y);
    x += glyphW + glyphSpacing;
  }
}

}

void pauseTextRender(SDL_Renderer *renderer) {
  constexpr int scale = 4;
  constexpr int letterW = glyphW * scale;
  constexpr int letterH = glyphH * scale;
  constexpr int spacing = glyphSpacing * scale;
  constexpr int count = sizeof(pausedLetterBits) / sizeof(pausedLetterBits[0]);
  constexpr int textW = count * letterW + (count - 1) * spacing;
  constexpr int x0 = (tilesPx.x - textW) / 2;
  constexpr int y0 = (tilesPx.y - letterH) / 2;

  SDL_CHECK(SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE));
  SDL_CHECK(SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255));
  int x = x0;
  for (int i = 0; i != count; ++i) {
    drawGlyph(renderer, pausedLetterBits[i], x, y0, scale);
    x += letterW + spacing;
  }
}

void hudRender(SDL_Renderer *renderer, SDL::QuadWriter &writer, entt::registry &reg) {
  const int hudY = tilesPx.y;

  SDL_CHECK(SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE));
  SDL_CHECK(SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255));
  const SDL_Rect strip{0, hudY, canvasPx.x, hudHeight};
  SDL_CHECK(SDL_RenderFillRect(renderer, &strip));

  const auto view = reg.view<Player, Lives, Score>();
  for (const entt::entity p : view) {
    const int remaining = view.get<Lives>(p).remaining;
    const int score = view.get<Score>(p).value;

    // Lives: one slot per starting life, drawn left to right. Remaining lives
    // are full-bright; spent ones are dimmed, so the lose screen shows how
    // many were used.
    for (int i = 0; i != startingLives; ++i) {
      const bool spent = i >= remaining;
      if (spent) {
        writer.setAlphaMod(immortalAlpha);
      }
      writer.tilePos({i * tileSize, hudY}, {tileSize, tileSize}, 0.0);
      writer.tileTex(lifeIconSprite);
      writer.render();
      if (spent) {
        writer.setAlphaMod(255);
      }
    }

    // Score: right-justified bitmap digits. White pixels. Minimum 2 digits so
    // a fresh game shows "00" rather than a single "0".
    SDL_CHECK(SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255));
    int digits[10];
    const int count = explodeDigits(score, digits, 2);
    const int textY = hudY + (hudHeight - glyphH) / 2;
    drawNumber(renderer, digits, count, canvasPx.x - 1 - numberWidth(count), textY);
  }
}

void summaryRender(SDL_Renderer *renderer, SDL::QuadWriter &writer, entt::registry &reg) {
  // Centered under the "you win" / "you lose" text baked into the end-screen
  // sprite: row of `startingLives` pacman icons (spent dim), score below.
  constexpr int gap = 4;
  constexpr int iconRowW = startingLives * tileSize;
  constexpr int summaryY = tilesPx.y - tileSize * 4;

  const auto view = reg.view<Player, Lives, Score>();
  for (const entt::entity p : view) {
    const int remaining = view.get<Lives>(p).remaining;
    const int score = view.get<Score>(p).value;

    int digits[10];
    const int count = explodeDigits(score, digits, 2);
    const int scoreW = numberWidth(count);

    const int iconsX = (tilesPx.x - iconRowW) / 2;
    const int scoreX = (tilesPx.x - scoreW) / 2;

    for (int i = 0; i != startingLives; ++i) {
      const bool spent = i >= remaining;
      if (spent) {
        writer.setAlphaMod(immortalAlpha);
      }
      writer.tilePos({iconsX + i * tileSize, summaryY}, {tileSize, tileSize}, 0.0);
      writer.tileTex(lifeIconSprite);
      writer.render();
      if (spent) {
        writer.setAlphaMod(255);
      }
    }

    SDL_CHECK(SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE));
    SDL_CHECK(SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255));
    drawNumber(renderer, digits, count, scoreX, summaryY + tileSize + gap);
  }
}
