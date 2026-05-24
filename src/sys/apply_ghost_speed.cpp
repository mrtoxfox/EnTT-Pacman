//
//  apply_ghost_speed.cpp
//  EnTT Pacman
//

#include "apply_ghost_speed.hpp"

#include "comp/dir.hpp"
#include "comp/ghost.hpp"
#include "comp/no_move.hpp"
#include "sys/can_move.hpp"
#include "comp/position.hpp"
#include "util/dir_to_pos.hpp"
#include "comp/prev_position.hpp"
#include "comp/ghost_speed_effect.hpp"
#include <entt/entity/registry.hpp>

void applyGhostSpeedExtraStep(entt::registry &reg, const MazeState &maze) {
  // SpeedEffect ghosts move a second tile in the same logic tick. Render
  // interpolates PrevPosition->Position, so the ghost slides two tiles over
  // the same tileSize frames -> doubled visual speed without a snap.
  auto view = reg.view<Ghost, SpeedEffect, Position, ActualDir>();
  for (const entt::entity e : view) {
    Pos &pos = view.get<Position>(e).p;
    const Dir dir = view.get<ActualDir>(e).d;
    if (dir == Dir::none) continue;
    if (!canMove(reg, maze, e, pos, dir)) continue;
    pos += toPos(dir);
    // Tunnel wrap. Duplicates the four-line rule from movement.cpp because
    // factoring a helper for two call sites isn't worth a new file.
    if (pos.y == 10) {
      bool wrapped = false;
      if (pos.x <= -1 && dir == Dir::left) {
        pos.x = 19;
        wrapped = true;
      } else if (pos.x >= 19 && dir == Dir::right) {
        pos.x = -1;
        wrapped = true;
      }
      if (wrapped && reg.has<PrevPosition>(e)) {
        reg.get<PrevPosition>(e).p = pos - toPos(dir);
      }
    }
  }
}

void applyGhostSpeedGate(entt::registry &reg) {
  // Clear last tick's NoMove tags first. 3.4.0 has no typed clear<T>(); the
  // iterate-and-remove pattern is what the rest of the codebase uses.
  {
    auto view = reg.view<NoMove>();
    for (const entt::entity e : view) {
      reg.remove<NoMove>(e);
    }
  }
  {
    auto view = reg.view<Ghost, FrozenEffect>();
    for (const entt::entity e : view) {
      reg.emplace<NoMove>(e);
    }
  }
  // Slow: gate every other tick by parity of the effect's own timer. Using the
  // SlowEffect timer (already decremented this tick) instead of the global
  // `ticks` keeps the cadence consistent across scatter/chase transitions,
  // which reset `ticks` to 0 and would otherwise double-step a slow ghost.
  {
    auto view = reg.view<Ghost, SlowEffect>();
    for (const entt::entity e : view) {
      if (view.get<SlowEffect>(e).timer % 2 == 0) {
        reg.emplace<NoMove>(e);
      }
    }
  }
}
