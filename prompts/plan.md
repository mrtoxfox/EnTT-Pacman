# Flashlight with 2D Shadow Casting - Plan

Working notes for `prompts/prompt.md`. Pure planning, no code yet. Targets the EnTT 3.4.0 / SDL2 / C++17 conventions in `CLAUDE.md`.

## Context recap

- Logical canvas: 152x184 px. Playfield is the top `tilesPx = 152x176`. HUD is the bottom 8-px strip.
- Maze grid: 19x22 tiles of 8 px. Walls (`Tile::wall`) and the ghost-house door (`Tile::door`) are opaque. Dots, energizers, and empty space are transparent.
- Loop is two-rate: `Game::logic` runs every 8 frames, `Game::render` runs every frame and receives `frame % tileSize` as the sub-tile pixel offset.
- Sprite facing in `playerRender` uses `DesiredDir` for the rotation angle and `ActualDir` for the sub-tile motion offset (`toPos(actualDir, frame)`).
- Pause is render-only: `state == paused` adds the dim overlay + PAUSED text; `pausedDebug` freezes the frame with no overlay. Both freeze `renderFrame` via `frozenFrame` already.
- Tunnel wraps only on the logic side (movement at `y=10`). Rendering does not wrap. The cone math must not wrap either.

## Step 1 - Branch: three strategies

### Strategy A - Ray-cast visibility polygon

Build the lit shape as two triangle fans (forward + back) in pixel space:

- Apex = player center in pixels, including sub-tile offset.
- Cast N rays per cone uniformly across `[-halfAngle, +halfAngle]` around facing.
- Walk each ray on the tile grid with DDA. Stop at the first `Tile::wall` or `Tile::door`, or at the configured beam length, whichever comes first. Out-of-maze tiles count as walls.
- Polygon vertices: apex + N+1 ray endpoints in order. Render as a triangle fan with `SDL_RenderGeometry`.
- Optionally augment by emitting 3 rays per wall corner inside the cone (corner angle, `+epsilon`, `-epsilon`) so the polygon vertices snap to true corners and shadow edges are pixel-crisp.

Compositing: render target texture, cleared to opaque black, draw both polygons with alpha 0 under `SDL_BLENDMODE_NONE`, blit back with `SDL_BLENDMODE_BLEND`.

### Strategy B - Tile-level BFS visibility mask

Treat lit tiles as a discrete set:

- From the player tile, BFS to neighbors through non-wall tiles only.
- For each visited tile, mark lit if its center lies inside the cone polygon (forward or back) and the path that reached it does not cross a wall.
- Render the darkness overlay, then carve out the lit tiles as 8x8 transparent quads.

### Strategy C - Shader-style stencil mask via 2D shadow volumes

Cone first, shadows second:

- Build the two cone polygons (forward, back) as plain wedge trapezoids in pixel space, no rays.
- Draw them transparent into a black render target as in A.
- For every wall tile whose AABB intersects either cone bbox, compute the two "silhouette" corners (the corners most extreme in angle from the apex). Extrude a black quad from those corners outward away from the apex, past the cone's far edge.
- Draw each shadow quad as opaque black with `SDL_BLENDMODE_NONE`, re-occluding the parts of the cone behind walls.

This is the classic 2D shadow-volume approach used by raycasters and dynamic-light demos.

## Step 2 - Evaluate

Scores 1 to 5, higher is better.

| Criterion | A: ray polygon | B: tile BFS | C: shadow volumes |
|-----------|---------------:|------------:|------------------:|
| Performance in two-rate loop | 4 | 5 | 3 |
| Visual quality (crisp edges, sub-tile slide) | 5 | 1 | 4 |
| Wall occlusion accuracy | 4 (5 with corner rays) | 3 | 4 |
| Code complexity | 4 | 5 | 2 |
| ECS / EnTT 3.4.0 fit and integration risk | 5 | 4 | 4 |
| **Total** | **22** | **18** | **17** |

Notes:

- A: per-frame work is N rays * shortish DDA walks + one `SDL_RenderGeometry` call per cone. At 30 fps with N=64 rays per cone and a 19x22 grid, this is hundreds of microseconds on any modern CPU. No allocations per frame if vertex storage is reused.
- B: BFS itself is fast, but tile-quantized lighting cannot slide smoothly with the sub-tile sprite offset. The cone outline visibly snaps in 8-px steps every logic tick. This fails the "smooth cone motion at sub-tile resolution" criterion in the prompt.
- C: correct in principle, but the boolean (cone minus shadow union) is fiddly. Care needed for walls that straddle the cone boundary, walls whose silhouette extrusion exits the playfield, and overlapping shadows. More code, more failure modes.

## Step 3 - Prune

Drop **B (tile BFS)**. Reason: the cone's apex must follow Pac-Man's interpolated pixel position; a tile-level mask can only resolve to 8-px steps, so the lit region snaps every tile boundary even though the sprite slides smoothly. That is the exact failure mode the prompt calls out under "sub-tile interpolation". The other criteria don't save it: even when the BFS path is correct, the rendered edge looks like Minecraft, not a flashlight.

## Step 4 - Deepen the two survivors

### A - Ray-cast visibility polygon (deepened)

**Cone geometry.**

- Forward cone: triangle fan with apex at the player center pixel, plus `N+1` perimeter vertices distributed across `[facing - halfFwd, facing + halfFwd]`. Far cap is `lengthFwd` tiles.
- Back cone: same construction, facing rotated 180 deg, with `halfBack` and `lengthBack`.
- Facing comes from the player's `DesiredDir`. That is what drives `playerRender`'s sprite rotation, so the cone visually matches the sprite. At standstill, `DesiredDir` keeps the last requested direction (consistent with the requirement "cone keeps whatever direction he was last facing").

**Apex math (sub-tile).**

```
apex.x = pos.x * tileSize + tileSize/2 + offset(actualDir, frame).x
apex.y = pos.y * tileSize + tileSize/2 + offset(actualDir, frame).y
```

`offset(actualDir, frame)` is what `playerRender` already uses via `toPos(ActualDir, frame)`. Using the same expression keeps the cone apex glued to the sprite center across all 8 sub-tile positions.

**Ray walk (DDA on tile grid).**

For each ray of angle `theta` and length cap `L`:

1. Convert `(cos theta, sin theta)` into an `(dx, dy)` step.
2. Walk the tile grid with the standard slab/DDA algorithm, alternately advancing to the next vertical or horizontal tile boundary, whichever is closer.
3. After each step, sample the tile. If `Tile::wall` or `Tile::door`, terminate at the exact intersection point with the wall edge. If outside the maze AABB, terminate at the AABB edge (this also disables tunnel-wrap leakage at `y=10`).
4. If accumulated distance exceeds `L * tileSize`, terminate at distance `L * tileSize`.

This gives sub-pixel-accurate hit points, which is what makes the edges crisp.

**Corner ray augmentation (for pixel-true edges).**

Pure uniform sampling at N=64 already looks clean at 152x176 logical resolution. For pixel-perfect shadow edges that converge to corners:

- For each wall corner inside the cone bbox, compute its angle from the apex. If the angle lies in the cone arc, emit three rays: at the corner angle and at `±0.5 degree`. The two epsilon rays "slip past" the corner on either side, producing the umbra/penumbra-free crisp wedge.
- Sort the union of (uniform rays + corner rays) by angle so the triangle-fan winding stays consistent.

Recommended starting point: 32 uniform rays per cone, no corner rays. Promote to corner rays only if visible aliasing shows up in playtesting.

**Narrow corridors, intersections, side branches.**

- Inside a 1-tile-wide corridor, rays clip on the side walls within a few pixels. The lit region narrows to the corridor width once the sprite moves into it.
- At a T-junction, the cone enters the perpendicular passage only along the angular slice that "sees through" the opening. Rays into the opening pass; rays into the wall stop on the wall. Side branches outside the cone's angular span stay dark by construction.
- A side branch fully inside the cone's angular span and not occluded by a wall lights up. A side branch occluded by a wall corner ends in a shadow wedge cast by that corner. Both behaviors are correct without special-case code.

**Tunnel at y=10.**

Maze AABB acts as the outer wall. Rays leaving the playfield horizontally on row 10 terminate at the screen edge. Light does not appear on the opposite side. This matches the prompt.

**Ghost-house door.**

Treated identically to `Tile::wall` in the ray walk's stop test. Light cannot enter the ghost house through the door.

**Per-frame cost.**

- 30 fps render: `(64 + 32) * 2 = 192` rays per frame in the starting config (forward 64, back 32). Each DDA walk is at worst `lengthFwd + 1` tile steps (~7) plus a few constant-time tile lookups. Order of low tens of microseconds per ray, total a few hundred microseconds.
- One `SDL_RenderGeometry` call per cone (2 calls per frame). The render target reuse means no allocation in steady state.
- Logic loop is untouched: the cone is render-only and pulls live data from the registry each render.

**Existing overlays.**

- `playing`, `paused`, `pausedDebug`: render flashlight after world sprites and before HUD. Pause overlay and PAUSED text both render after the flashlight, so the text stays bright over the darkness.
- `won`, `lost`: skip the flashlight entirely. The end screens use `fullRender(SpriteID::win|lose)` plus `summaryRender` and stay fully lit (matches the existing behavior on game end).
- HUD strip (`y >= tilesPx.y`) is excluded by setting the render-target texture size to `tilesPx` and blitting only into the playfield rect.

**Pause behavior.**

`Game::render` already freezes `renderFrame` to `frozenFrame` outside `playing`. The flashlight uses `renderFrame` for the sub-tile apex offset, so the cone freezes with the sprite. The world state (player Position, DesiredDir, ActualDir) does not change while paused, so the cone polygon is identical frame-to-frame during the freeze. PAUSED text overdraws cleanly.

**Sprite coverage at the apex.**

The forward cone starts at the apex (a single point), so very close to the apex the lit width is near zero. Pac-Man's sprite is 8x8 centered on the apex. To guarantee his sprite is fully lit:

- Pick `lengthFwd` and `halfFwd` so the cone width at distance 4 px exceeds 8 px. With `halfFwd = 35 deg`, the width at 4 px is `2 * 4 * tan(35) ~= 5.6 px` - too narrow.
- Solution: the back cone, with `halfBack = 75 deg` and `lengthBack = 2`, plus a forward cone with `halfFwd = 30 deg`, jointly cover the apex disc. The union of the two fans at distance 4 px from the apex spans roughly 360 deg minus two narrow side wedges (about 60 deg total), enough to cover the sprite center. Pac-Man's 8x8 sprite extends 4 px in any cardinal direction, so the worst case is the two "side" wedges directly perpendicular to facing.

If a 4-px side strip of the sprite peeks into the dark, fall back to a one-time fix: add a tiny circular safe disc (radius 6 px) at the apex as an extra polygon. The prompt explicitly allows this ("no extra halo logic unless the cone math fails to cover his sprite"). Plan to inspect this in the first build and only add the disc if the gap is visible.

**Dots and energizers do not affect lighting.**

The flashlight system reads the player's `Position` and `DesiredDir`, the maze grid for occlusion, and nothing else. Energizer pickup, ghost mode, score - all irrelevant. The cone constants in `constants.hpp` are fixed.

**Wall edges inside the lit region.**

The maze is one large sprite (`SpriteID::maze`) drawn in `fullRender` before the flashlight. The flashlight only modulates alpha via the overlay; it never touches the underlying RGB. So wall edges render as their normal continuous lines wherever the overlay punches a hole, regardless of where the cone arc falls.

**SDL2 compositing.**

- Render target texture: `SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, tilesPx.x, tilesPx.y)`. Mark `SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND)` so the final blit honors alpha.
- Cache the texture on `Game` (the prompt explicitly permits this as the one allowed piece of non-ECS state). Recreate only if the renderer is lost.
- Per frame:
  1. `SDL_SetRenderTarget(renderer, overlay)`.
  2. `SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE)`.
  3. `SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); SDL_RenderClear(renderer);`.
  4. Build vertex arrays for forward + back cones. Each vertex has `color = {0, 0, 0, 0}` (transparent black). Call `SDL_RenderGeometry(renderer, nullptr, verts, n, indices, m)` once per cone.
  5. `SDL_SetRenderTarget(renderer, nullptr)`.
  6. `SDL_RenderCopy(renderer, overlay, nullptr, &playfieldRect)` where `playfieldRect = {0, 0, tilesPx.x, tilesPx.y}`.
- `SDL_RenderGeometry` is available since SDL 2.0.18 (released 2022-01). Homebrew, current Debian/Ubuntu, current vcpkg, and AppVeyor's vcpkg all ship newer SDL2. The README and PR description must record this minimum version.

Triangle-fan as indexed triangles: for `N+1` rim vertices plus apex at index 0, indices are `(0, 1, 2), (0, 2, 3), ..., (0, N, N+1)`.

### C - 2D shadow volumes (deepened, for honest comparison)

**Geometry.**

- Cone polygons: two trapezoids built once per frame as in the prompt's `flashlight.md` original sketch. No rays.
- For each wall tile with AABB overlapping the cone bbox:
  - Compute the angle from the apex to each of the wall's 4 corners.
  - Pick the two corners with extreme angles (one CW-most, one CCW-most). These are the silhouette.
  - Extrude each silhouette corner radially outward by `extrudeLen` (must exceed cone length so the shadow exits the cone).
  - Form a quad: `[corner1, corner2, extrude2, extrude1]`.

**Rendering order on the render target.**

1. Clear with opaque black.
2. Draw forward cone polygon and back cone polygon as transparent (alpha 0) with `SDL_BLENDMODE_NONE`. Cone-shaped holes appear.
3. For each shadow quad, draw it as opaque black with `SDL_BLENDMODE_NONE`. Each quad "reseals" the part of the cone hole that lies behind a wall.
4. Blit to screen with `SDL_BLENDMODE_BLEND`.

**Edge cases.**

- Walls straddling the cone boundary: the silhouette corners may lie on either side of the cone arc. The shadow quad can extend beyond the cone in pixel space; that is fine because the surrounding pixels are already opaque black from the initial clear.
- Walls behind the apex when only the back cone is active: same logic, smaller scale.
- Adjacent walls in a row: each contributes its own shadow quad. Quads overlap. With opaque black and `BLENDMODE_NONE`, overlap is idempotent. No artifact.
- Concave/L-shaped wall blocks: each wall tile is processed independently. Inner corners are handled correctly because the silhouette picks the extreme angles per tile.

**Cost.**

- Cone setup: 2 polygons, 4 vertices each.
- Shadow quads: typical cone covers 20 to 40 wall tiles in this maze; each contributes 1 quad (4 vertices, 6 indices). 40 quads = 160 vertices, 240 indices.
- Two `SDL_RenderGeometry` calls (one for cones, one for shadow quads, batched). Cheap.

**Why this is still the runner-up.**

- Three passes (clear, cone, shadow) on the render target, vs A's two passes (clear, cones).
- Silhouette logic is correct but easy to get subtly wrong, and bugs show up as light leaking through walls. A's "stop the ray at the first wall" is one well-understood algorithm with one obvious failure mode (too few rays).
- Sub-tile smoothness for both is identical (apex is interpolated in pixel space either way).
- C does no occlusion on the cone arc itself, only on cone interior. That is actually fine for this game (the arc is a "light source", not a real surface), but is a conceptual asymmetry.

## Step 5 - Select

**Winner: A (ray-cast visibility polygon).**

Direct comparison vs C:

- Fewer geometry primitives per frame (~64 ray endpoints vs ~40 wall quads on a typical busy frame).
- Single triangle fan per cone, no boolean stitching. C needs a clear-then-carve-then-uncarve pass.
- A's worst-case failure mode is "the ray count is too low, edges look polygonal" - cheap to tune.
- C's worst-case failure mode is "the silhouette picker misclassifies an edge case, light leaks through a corner" - harder to debug.
- The corner-ray augmentation in A converges to the same crisp-edge result as C in the limit, but starts cleaner without it.
- A scales naturally if a second flashlight ever appears (a ghost with a light): one more triangle fan, done.

C is fine and well-known. A is a better fit for the prompt's "visibility polygon from Pac-Man's position" framing in step 1 of the prompt itself.

### Recommended tunable values

| Tunable | Value | Reason |
|---------|------:|--------|
| `flashlightForwardTiles` | 6 | Lights ~6 tiles ahead; reads as a flashlight beam, not a spotlight or a torch. |
| `flashlightBackTiles` | 2 | Matches the small "stub" in the reference; covers the apex side. |
| `flashlightForwardHalfDeg` | 22 | Total forward arc 44 deg, narrow beam. Wide enough to see one tile to each side at 6 tiles range (`2 * 6 * 8 * tan(22) ~= 39 px ~= 5 tiles`). |
| `flashlightBackHalfDeg` | 75 | Total back arc 150 deg. Wide stub plus forward 44 deg covers Pac-Man's 8x8 sprite at the apex with margin. |
| `flashlightForwardRays` | 48 | At 6-tile cap and 44 deg arc, angular resolution is ~1 deg per ray, far below 1-pixel discrimination. |
| `flashlightBackRays` | 24 | Same angular density (~6 deg per ray? No - 150/24=6.25 deg per ray; back cone is short so even coarse rays are pixel-clean). |
| `darknessAlpha` | 255 | Pure black outside the beam (prompt mandate). |

Total rays per frame: 72. Generous, well under any performance concern.

## Step 6 - Implementation plan

### Files

- `src/comp/flashlight.hpp` - new, empty tag struct.
- `src/core/constants.hpp` - add 7 tunables.
- `src/core/factories.cpp` - attach `Flashlight` to the player entity.
- `src/sys/flashlight_render.hpp` and `flashlight_render.cpp` - new render system. Free functions, first param `entt::registry &`, additional params for the SDL renderer, the cached overlay texture, and the maze.
- `src/core/game.hpp` - add a cached overlay `SDL::Texture` member (the one allowed piece of non-ECS state).
- `src/core/game.cpp` - call `flashlightRender(...)` from `Game::render` in the right place; create the overlay texture lazily on first use (or in a small init helper that takes the renderer).

The CMake `file(GLOB_RECURSE)` picks up the new files automatically. No `CMakeLists.txt` edit needed.

### Step-by-step

1. **Add the tag component.**
   ```cpp
   // src/comp/flashlight.hpp
   #ifndef COMP_FLASHLIGHT_HPP
   #define COMP_FLASHLIGHT_HPP
   struct Flashlight {};
   #endif
   ```
   Plain aggregate, empty tag, header-only. Matches the other tag components.

2. **Attach it to the player.** In `core/factories.cpp::makePlayer`, add `reg.emplace<Flashlight>(e);` next to the other component emplacements. No factory signature change.

3. **Add the tunables to `core/constants.hpp`.**
   ```cpp
   // Flashlight: forward beam length in tiles.
   constexpr int flashlightForwardTiles = 6;
   // Flashlight: back beam length in tiles.
   constexpr int flashlightBackTiles = 2;
   // Flashlight: forward cone half-angle in degrees.
   constexpr int flashlightForwardHalfDeg = 22;
   // Flashlight: back cone half-angle in degrees.
   constexpr int flashlightBackHalfDeg = 75;
   // Flashlight: number of perimeter rays for the forward cone.
   constexpr int flashlightForwardRays = 48;
   // Flashlight: number of perimeter rays for the back cone.
   constexpr int flashlightBackRays = 24;
   // Alpha of the darkness overlay outside the beam (0..255). 255 = pure black.
   constexpr std::uint8_t flashlightDarknessAlpha = 255;
   ```
   All `constexpr`, all in the existing constants header. Includes already pull `<cstdint>` transitively through `util/dir.hpp` (verify; if not, add `#include <cstdint>`).

4. **Cache the overlay texture on `Game`.** In `core/game.hpp`, add:
   ```cpp
   SDL::Texture flashlightOverlay;
   ```
   Use the existing `SDL::Texture` RAII wrapper (`util/sdl_delete.hpp`). The destructor frees the GPU resource at shutdown.

5. **Write the flashlight render system.** New files `src/sys/flashlight_render.hpp` and `.cpp`. Signature:
   ```cpp
   void flashlightRender(
     entt::registry &reg,
     SDL_Renderer *renderer,
     SDL_Texture *overlay,
     const MazeState &maze,
     int frame
   );
   ```
   Body (sketch):
   - Find the player entity via `reg.view<Flashlight, Position, ActualDir, DesiredDir>()`. There is exactly one. If empty, return.
   - Compute apex in pixels: `pos.p * tileSize + Pos{tileSize/2, tileSize/2} + toPos(actualDir, frame)`.
   - Pick facing radians from `DesiredDir`. Map: `right=0, down=pi/2, left=pi, up=3pi/2` (matches the existing 90-deg rotation convention in `playerRender`).
   - Build forward polygon: for `i = 0..flashlightForwardRays`, ray angle `= facing + lerp(-halfFwd, +halfFwd, i / N)`. Walk DDA on the maze grid, capped at `flashlightForwardTiles * tileSize`. Record endpoint.
   - Build back polygon: same with `facing + pi` and the back tunables.
   - Set render target to `overlay`. Clear opaque black. With `SDL_BLENDMODE_NONE`, draw both polygons as transparent (alpha 0) via two `SDL_RenderGeometry` calls.
   - Restore render target to default (`SDL_SetRenderTarget(renderer, nullptr)`).
   - `SDL_RenderCopy` the overlay to `{0, 0, tilesPx.x, tilesPx.y}` with the overlay's blend mode set to `SDL_BLENDMODE_BLEND`.

   DDA helper lives in an anonymous namespace inside `flashlight_render.cpp` (per `CLAUDE.md`'s rule on file-local helpers). It takes `(apexX, apexY, dirX, dirY, maxPx, maze)` and returns the endpoint in pixel coordinates. Treat `Tile::wall`, `Tile::door`, and out-of-maze as opaque.

   Vertex arrays: `std::vector<SDL_Vertex>` and `std::vector<int>` as static locals inside the function (`static thread_local` to avoid reallocation per frame). Acceptable per `CLAUDE.md`'s "no new state outside the ECS" rule because they are scratch graphics buffers, not game state, the same way the overlay texture is.

6. **Wire it into `Game::render`.** In `core/game.cpp`:
   - On the first call (when `flashlightOverlay` is null), create the overlay: `SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, tilesPx.x, tilesPx.y)` and `SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND)`. Wrap in `SDL::Texture`.
   - Adjust the playing/paused branch:
     ```cpp
     fullRender(writer, animera::SpriteID::maze);
     dotRender(writer, maze);
     playerRender(reg, writer, renderFrame);
     ghostRender(reg, writer, renderFrame);
     flashlightRender(reg, renderer, flashlightOverlay.get(), maze, renderFrame);
     if (state == State::paused) {
       pauseOverlayRender(renderer);
       pauseTextRender(renderer);
     }
     hudRender(renderer, writer, reg);
     ```
   - The `won` and `lost` branches are untouched, so the end screens are unaffected.

7. **Validate sprite coverage at the apex.** Build Debug, run, walk in all four directions, confirm Pac-Man is fully visible. If a sliver of his sprite slips into the dark on one cardinal, add a tiny opaque-transparent disc at the apex as an additional `SDL_RenderGeometry` polygon (12-vertex regular polygon, radius 6 px). Keep this as a single conditional code block, not a tunable, unless multiple values need testing.

8. **Verify under `-Wall -Wextra -Wpedantic`.** Run:
   ```
   cd build && cmake -DCMAKE_BUILD_TYPE=Debug .. && cmake --build .
   ```
   Fix any sign-compare or unused-parameter warnings. The DDA loop and trig math are the likely sources.

9. **Capture screenshots.** Run Release, walk to a spot where each cardinal direction has a visible wall casting a shadow. Capture four PNGs (one per direction). Stash them in a place the PR can reference.

10. **PR description.** Include:
    - SDL2 polygon-fill call used: `SDL_RenderGeometry`.
    - SDL2 minimum version: 2.0.18 (released 2022-01).
    - Tunable values chosen (the table from Step 5).
    - One-paragraph description of the cone polygon construction (apex + uniform ray endpoints, DDA wall stop, treat door as wall, no tunnel wrap).
    - Note that ghost AI and collision are untouched.

### Risk register

- **`SDL_RenderGeometry` not present**: would mean SDL2 < 2.0.18. Mitigation: detect at build time via `SDL_VERSION_ATLEAST(2, 0, 18)`; if absent, fall back to per-scanline `SDL_RenderFillRect`s of the polygon rows. Document the minimum version in the PR description so packagers know.
- **Render-target lost on window resize / device reset**: the game uses `SDL_RenderSetLogicalSize`, so the logical target dimensions are stable. The window does not resize at runtime. No recreation logic needed in the base case.
- **Trig precision at tile boundaries**: DDA is integer-stepped with floating-point distances; the endpoint can be 0.5-px past the wall. Visually invisible at logical resolution 8x. If it leaks at integer-scaled large windows, clamp the endpoint to the wall plane exactly.
- **Pause + flashlight interaction**: relies on `Game::render` already feeding `frozenFrame` while paused. Confirmed in the current code. No extra work.

### Out of scope (mirror the prompt)

Battery, color, soft edges, energizer modes, 360-deg halo, gameplay changes. The plan touches rendering only.
