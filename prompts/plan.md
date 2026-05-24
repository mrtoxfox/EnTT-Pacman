# Fog of War

**Goal:** Hide unvisited maze areas. Tiles reveal permanently as Pac-Man moves near them. Ghosts standing on unrevealed tiles are invisible. Dots and energizers under fog are hidden too.

**Architecture:** A `Grid<bool>` parallel to the maze tile grid, stored on `Game` alongside `maze`. A new free-function system updates it from the player's position each logic step. A new free-function render helper draws an opaque black 8x8 rect over each unrevealed tile. `ghostRender` skips ghosts whose tile is unrevealed. No new dependencies.

---

## 1. Variants considered

### A. Grid<bool> + per-tile black quad overlay

- Store `Grid<bool> fog` alongside the maze tile grid.
- Each logic tick, reveal tiles within a Chebyshev radius of the player.
- After the world is drawn, fill every unrevealed tile with an opaque black 8x8 rect.
- Ghost render skips entities whose tile is unrevealed.

### B. Pixel-level alpha texture + radial gradient

- One `SDL_Texture` sized to the maze (152x176), 32-bit RGBA, starts fully opaque black.
- Reveal: lock the texture, paint a radial alpha falloff around the player, taking `min(current_alpha, gradient_alpha)` so cleared pixels stay cleared.
- Render: world + sprites, then blit the fog texture over the maze with `SDL_BLENDMODE_BLEND`.
- Ghost visibility: CPU-side mirror of the alpha channel; a ghost is drawn iff `mirror[ghost_pixel] < threshold`.

### C. Per-tile fog entities in the ECS

- One entity per maze tile holding a `Fog` tag and `Position`, created in `Game::init`.
- A system finds fog entities within Chebyshev radius of the player and destroys them.
- Render iterates `view<Fog, Position>` and draws an 8x8 black rect per remaining fog entity.
- Ghost visibility: scan the view (or maintain a spatial index) to check whether a fog entity sits on the ghost's tile.

---

## 2. Scoring (1-5; for "Complexity" higher = simpler)

| Strategy | Performance | Visuals | Complexity | ECS / CLAUDE.md fit |
|----------|-------------|---------|------------|---------------------|
| A        | 5           | 2       | 5          | 4                   |
| B        | 3           | 5       | 2          | 2                   |
| C        | 4           | 2       | 3          | 3                   |

Notes:
- Perf: A and C both worst-case ~418 fills; A wins on no view iteration cost. B's pixel writes happen at logic rate so amortise OK, but the CPU mirror adds bookkeeping.
- Visuals: B is the only one with smooth fog. A and C are tile-grid sharp.
- Complexity: A is one grid field + one reveal helper + one render loop. B adds texture format choice, blend modes, locked-pixel writes, a CPU mirror, and a new resource lifetime. C adds 418 entities, a spatial index for ghost queries, and view churn.
- ECS fit: CLAUDE.md keeps grid-shaped state outside the ECS (maze is the canonical exception; tunnel and house door are explicitly not entities). A reuses that pattern. C violates it. B introduces a resource type outside the QuadWriter quad pipeline.

---

## 3. Pruned

**C is eliminated.** Same hard-edged visuals as A, worse perf, worse fit. CLAUDE.md explicitly avoids entity-per-cell modeling for exactly this shape of state. C buys nothing A doesn't already give us.

---

## 4. Deepened

### A: Grid<bool> + per-tile quad

| Question | Answer |
|----------|--------|
| Reveal radius | 2 tiles Chebyshev (5x5 square around the player). |
| Reset between lives | No. Fog persists across deaths. |
| Reset on level restart | Yes. `Game::init` rebuilds the grid all-false. |
| Tunnel wrap at y=10 | `revealAround` special-cases that row: x indices that fall off either side wrap to the opposite end. Same literal-coordinate shortcut already used in `sys/movement.cpp` and `sys/can_move.cpp`. |
| Ghost house | Starts fogged. Reveals only when Pac-Man wanders close enough. |
| Dots / energizers | Fully hidden. They render first; the fog quad paints over them. Win still works (it counts eaten dots, not visible ones). |
| Ghosts crossing the fog edge | Tile-granular. The tick a ghost steps onto a revealed tile, it pops in at the tile edge. One logic tick of pop (~267 ms). Acceptable. |
| Fog edge appearance | Tile-aligned hard squares. Matches the 8-pixel maze aesthetic. |
| Score / win-lose interaction | Fog draws only in `playing` and `paused`. Win/lose use `fullRender` over the maze area; HUD strip is outside the fog area. No interaction. |
| State cost / location | 418 bools = 53 bytes. Lives as `Grid<bool> fog` on `Game`. Same documented "outside ECS" reason as the maze itself. |

### B: pixel alpha texture + radial gradient

| Question | Answer |
|----------|--------|
| Reveal radius | 16 px (2 tiles), with a 4 px soft falloff at the edge. |
| Reset between lives | No. |
| Reset on level restart | Yes. Refill the texture with opaque black. |
| Tunnel wrap at y=10 | Reveal at that row also writes the opposite-side pixels of the texture. |
| Ghost house | Starts fogged. |
| Dots / energizers | At the fog edge they're partially visible. Inside fog, fully hidden. |
| Ghosts crossing the fog edge | Smooth. Ghost drawn with alpha matching the CPU mirror at its position; fades in pixel by pixel. |
| Fog edge appearance | Smooth, soft. |
| Score / win-lose interaction | Same as A. |
| State cost / location | One `SDL_Texture` (~107 KB GPU) + a CPU alpha mirror (~26 KB). New `FogState` RAII type. Resource lifetime outside the current pipeline. |

---

## 5. Selected: A

Direct comparison:
- *Visuals.* B looks better in isolation, but the maze is 8-px tile pixel art. A smooth radial gradient would clash with the hard sprite edges next to it. A's tile-aligned fog fits the aesthetic.
- *Code shape.* A is one new grid field, one reveal helper, one render loop. B is a new RAII resource, an `SDL_PIXELFORMAT` choice, locked-pixel writes, blend mode setup, a CPU mirror, plus the same reveal helper. Roughly five times the code for a visual upgrade the art style doesn't reward.
- *CLAUDE.md fit.* A drops into the existing maze-grid exception. B introduces a new resource lifetime outside the QuadWriter quad pipeline.
- *Perf.* A is cheaper, bounded (~418 fills worst case, dropping as the player explores).

---

## 6. Implementation plan

### Task 1: constants

In `src/core/constants.hpp`:

```cpp
// Chebyshev radius (in tiles) around the player that reveals from fog of war.
constexpr int fogRevealRadius = 2;
// Tile-row y of the tunnel wrap-around. Already implicit in movement.cpp /
// can_move.cpp; centralised here so revealFog can wrap too.
constexpr int tunnelRow = 10;
```

(Decision to centralise `tunnelRow`: the literal `10` already appears twice in the codebase. Pulling it into one constant is a small cleanup that earns its keep because the new `revealAround` needs it.)

### Task 2: fog state on Game

In `src/core/game.hpp`, add a member next to `maze`:

```cpp
MazeState maze;
Grid<bool> fog;
```

Include `util/grid.hpp` if not already in via `maze.hpp` (it is).

### Task 3: revealAround + revealFog system

New files `src/sys/reveal_fog.hpp` and `src/sys/reveal_fog.cpp`.

Header:

```cpp
#ifndef SYS_REVEAL_FOG_HPP
#define SYS_REVEAL_FOG_HPP

#include "util/grid.hpp"
#include <entt/entity/fwd.hpp>

void revealFog(entt::registry &, Grid<bool> &fog);

#endif
```

Impl:

```cpp
#include "reveal_fog.hpp"

#include "core/constants.hpp"
#include "comp/player.hpp"
#include "comp/position.hpp"
#include <entt/entity/registry.hpp>

namespace {

void markTile(Grid<bool> &fog, const int x, const int y) {
  if (x >= 0 && x < fog.width() && y >= 0 && y < fog.height()) {
    fog[{x, y}] = true;
  }
}

void revealAround(Grid<bool> &fog, const Pos center) {
  const int w = fog.width();
  for (int dy = -fogRevealRadius; dy <= fogRevealRadius; ++dy) {
    const int y = center.y + dy;
    for (int dx = -fogRevealRadius; dx <= fogRevealRadius; ++dx) {
      const int x = center.x + dx;
      markTile(fog, x, y);
      // Tunnel wrap: at row 10, x indices that fall off either side wrap to
      // the opposite end. Same hard-coded shortcut as movement.cpp.
      if (y == tunnelRow) {
        if (x < 0) markTile(fog, x + w, y);
        else if (x >= w) markTile(fog, x - w, y);
      }
    }
  }
}

}

void revealFog(entt::registry &reg, Grid<bool> &fog) {
  const auto view = reg.view<Player, Position>();
  for (const entt::entity e : view) {
    revealAround(fog, view.get<Position>(e).p);
  }
}
```

### Task 4: initialise fog state and reveal at spawn

In `Game::init` (`src/core/game.cpp`):

```cpp
void Game::init(Audio &device) {
  maze = makeMazeState();
  fog = Grid<bool>{tiles};  // tiles is the Pos constant from constants.hpp
  const entt::entity player = makePlayer(reg);
  // ... existing ghost factories ...
  rand.seed(std::random_device{}());
  revealFog(reg, fog);       // reveal around spawn so frame 1 isn't blank
  device.playMusic(SoundId::intro, false);
}
```

Add `#include "sys/reveal_fog.hpp"`.

### Task 5: call revealFog in the logic loop

In `Game::logic`, after `wallCollide(reg, maze)`:

```cpp
movement(reg);
wallCollide(reg, maze);
revealFog(reg, fog);
dots += eatDots(reg, maze);
// ...
```

Placement reasoning: player position is finalised by `wallCollide`. Reveal runs before anything that might want to consult visibility. `audio(reg, device)` stays last per CLAUDE.md.

### Task 6: fogRender

In `src/sys/render.hpp`:

```cpp
// Fills every unrevealed tile with an opaque black 8x8 rect. Call after the
// world and ghosts, before the pause overlay.
void fogRender(SDL_Renderer *, const Grid<bool> &fog);
```

In `src/sys/render.cpp`:

```cpp
void fogRender(SDL_Renderer *renderer, const Grid<bool> &fog) {
  SDL_CHECK(SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE));
  SDL_CHECK(SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255));
  for (int y = 0; y != fog.height(); ++y) {
    for (int x = 0; x != fog.width(); ++x) {
      if (!fog[{x, y}]) {
        const SDL_Rect rect{x * tileSize, y * tileSize, tileSize, tileSize};
        SDL_CHECK(SDL_RenderFillRect(renderer, &rect));
      }
    }
  }
}
```

### Task 7: hide ghosts under fog

Change `ghostRender` signature to take `const Grid<bool> &fog`. In the view loop, skip ghosts whose tile is unrevealed:

```cpp
void ghostRender(entt::registry &reg, SDL::QuadWriter &writer,
                 const Grid<bool> &fog, const int frame) {
  const auto view = reg.view<Position, ActualDir, GhostSprite>();
  for (const entt::entity e : view) {
    const Pos tilePos = view.get<Position>(e).p;
    if (!fog[tilePos]) continue;
    // ... existing draw code ...
  }
}
```

Update the call site in `Game::render`.

### Task 8: wire fogRender into the frame

In `Game::render`:

```cpp
if (state == State::playing || state == State::paused) {
  const int renderFrame = (state == State::paused) ? 0 : frame;
  fullRender(writer, animera::SpriteID::maze);
  dotRender(writer, maze);
  playerRender(reg, writer, renderFrame);
  ghostRender(reg, writer, fog, renderFrame);
  fogRender(renderer, fog);
  if (state == State::paused) {
    pauseOverlayRender(renderer);
  }
  hudRender(renderer, writer, reg);
} else if (state == State::won) {
  // ... unchanged ...
}
```

Player is drawn before fog so the small black square around the player is dominated by reveal anyway (player's tile is always revealed). Could alternatively draw player on top of fog; current order is simpler.

### Task 9: build and verify

```
cd build
cmake --build .
./pacman
```

Manual checks:
- New game: only the 5x5 around spawn is visible.
- Walk; tiles reveal and stay revealed.
- Tunnel: both ends of row 10 reveal as the player approaches either edge.
- Approach the ghost house: it reveals at 2 tiles distance.
- A ghost crossing into a revealed tile pops in at the tile boundary (one-tick pop, acceptable).
- Pause: fog and pause overlay both render; the overlay covers the maze area uniformly.
