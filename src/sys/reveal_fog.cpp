//
//  reveal_fog.cpp
//  EnTT Pacman
//

#include "reveal_fog.hpp"

#include "core/constants.hpp"
#include "comp/player.hpp"
#include "comp/position.hpp"
#include <entt/entity/registry.hpp>

// Fog is stored as Grid<std::uint8_t> (0 = unrevealed, 1 = revealed) rather
// than Grid<bool>, because Grid::operator[] returns `const Elem&` and
// std::vector<bool>'s proxy reference can't bind to that. Writes like
// `fog[pos] = true` would silently target a temporary.

namespace {

void markTile(Grid<std::uint8_t> &fog, const int x, const int y) {
  const Pos p{x, y};
  if (!fog.outOfRange(p)) {
    fog[p] = 1;
  }
}

// Reveals fog cells around the player. Fog is stored at fogSubdiv x fogSubdiv
// resolution per maze tile, so all coordinates here are in cell-space. The
// reveal box covers the player's own fogSubdiv x fogSubdiv block plus
// fogRevealRadius tiles (= fogRevealRadius * fogSubdiv cells) in each
// direction. The on-screen reach matches the tile-resolution version exactly;
// only the granularity of the edge changes.
void revealAround(Grid<std::uint8_t> &fog, const Pos centerTile) {
  const int w = fog.width();
  const int cellRadius = fogRevealRadius * fogSubdiv;
  const int cx0 = centerTile.x * fogSubdiv;
  const int cy0 = centerTile.y * fogSubdiv;
  const int tunnelCellY0 = tunnelRow * fogSubdiv;
  for (int dy = -cellRadius; dy < fogSubdiv + cellRadius; ++dy) {
    const int y = cy0 + dy;
    for (int dx = -cellRadius; dx < fogSubdiv + cellRadius; ++dx) {
      const int x = cx0 + dx;
      markTile(fog, x, y);
      // Tunnel wrap: on the cell rows covering tile row tunnelRow, x indices
      // off either side wrap to the opposite end. Same shortcut used by
      // sys/movement.cpp.
      if (y >= tunnelCellY0 && y < tunnelCellY0 + fogSubdiv) {
        if (x < 0) markTile(fog, x + w, y);
        else if (x >= w) markTile(fog, x - w, y);
      }
    }
  }
}

}

void revealFog(entt::registry &reg, Grid<std::uint8_t> &fog) {
  const auto view = reg.view<Player, Position>();
  for (const entt::entity e : view) {
    revealAround(fog, view.get<Position>(e).p);
  }
}
