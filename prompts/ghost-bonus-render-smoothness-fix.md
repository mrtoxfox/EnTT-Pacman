# Ghost speed-bonus render smoothness fix

## Symptom

After the universal bonus system shipped, two visual bugs showed up on ghosts under speed effects:

- **Speed**: ghosts "teleport" forward instead of sliding at double speed.
- **Slow**: ghosts flicker (slide forward, then snap back) instead of sliding at half speed.

Frozen ghosts (which simply stop) and unaffected ghosts looked correct.

## Root cause

The original render code forward-extrapolated motion from the current tile:

```cpp
writer.tilePos(pos * tileSize + toPos(actualDir, frame), ...);
```

That math assumes one tile of movement per logic tick in the direction of `actualDir`. It breaks once the bonus system makes that no longer true:

- `applyGhostSpeedExtraStep` moves a `SpeedEffect` ghost a **second** tile in the same tick. The render still slides forward by only one tile of pixels, then jumps when the next tick lands two tiles ahead. Result: teleport.
- `applyGhostSpeedGate` adds `NoMove` on alternating ticks for `SlowEffect`. On a `NoMove` tick `Position` doesn't change, but the renderer still extrapolates `pos + frame` in `actualDir`, sliding the sprite forward. Next tick `Position` is still the same tile, so the extrapolation restarts from the same origin — the sprite snaps back. Result: flicker.

The renderer needs to interpolate between the actual start-of-tick and end-of-tick positions, not extrapolate from the current tile.

## Fix

Snapshot each entity's position at the start of the logic tick, then have the renderer linearly interpolate from that snapshot to the current position over `tileSize` render frames.

### New component

`src/comp/prev_position.hpp`

```cpp
struct PrevPosition { Pos p; };
```

### New system

`src/sys/update_prev_position.{hpp,cpp}` — copies `Position` into `PrevPosition` for every entity that has both. Runs as the **first** call in `Game::logic` so every later position mutation (movement, the speed extra step, anything else) is captured as the post-snapshot delta.

### Factory wiring

`src/core/factories.cpp` — every entity that moves now also gets a `PrevPosition`:

- `makePlayer` emplaces `PrevPosition{playerSpawnPos}`.
- `makeGhost` emplaces `PrevPosition{home}`.
- `makeBlinky` overrides both `Position` and `PrevPosition` to `outsideHouse`, matching the existing `Position` override.

### Logic order

`src/core/game.cpp` — `updatePrevPosition(reg)` is now the first system called inside `Game::logic`, right after the scatter/chase phase advance and before `movement`.

### Tunnel wrap handling

The tunnel teleports `pos.x` between `-1` and `19` in a single tick. Naive interpolation would drag the sprite across the entire maze on the wrap tick.

In both `src/sys/movement.cpp` and `src/sys/apply_ghost_speed.cpp` (`applyGhostSpeedExtraStep`), after a tunnel wrap we overwrite `PrevPosition` to one tile past the wrapped tile in the pre-wrap direction:

```cpp
if (wrapped && reg.has<PrevPosition>(e)) {
  reg.get<PrevPosition>(e).p = pos - toPos(dir);
}
```

That makes the sprite slide onto the destination edge over the wrap tick, matching the original visual continuity (sprite enters from the far edge and continues at the same speed) instead of snapping or dragging.

### Renderer

`src/sys/render.cpp` — both `playerRender` and `ghostRender` now read `PrevPosition` alongside `Position` and call a shared helper:

```cpp
Pos interpRenderPx(const Pos prev, const Pos curr, const int frame) {
  const Pos prevPx = prev * tileSize;
  const Pos currPx = curr * tileSize;
  return {
    prevPx.x + (currPx.x - prevPx.x) * frame / tileSize,
    prevPx.y + (currPx.y - prevPx.y) * frame / tileSize,
  };
}
```

Per-component math because `Pos` doesn't have a `/int` operator. `actualDir` is still read by ghosts for sprite-direction selection (`dirOffset`), but it no longer drives the slide.

### Slow-gate stability

While in the area, `applyGhostSpeedGate` had a subtle bug independent of rendering: it gated `SlowEffect` ghosts when `tick % 2 == 0`, where `tick` is the global `ticks` counter — but `ticks` resets to `0` on every scatter/chase phase transition. A reset mid-effect could swap parity and double-step a slow ghost.

The gate now keys off `SlowEffect.timer` parity instead. Since the timer is monotonically decreasing and lives on the effect itself, the cadence is consistent across phase transitions.

## Why this also helps general motion

Even without bonuses, the new model is strictly better than forward extrapolation:

- It removes the dependency on `actualDir` matching the actual movement that just happened. `wallCollide` can now flip `actualDir` without the renderer producing a sub-tile slide in the wrong direction.
- It makes future "non-uniform per-tick movement" effects (e.g. teleporters, dashes, knockback) renderable without per-effect render hacks — they just need to touch `Position` like everything else.

## Files touched

New:

- `src/comp/prev_position.hpp`
- `src/sys/update_prev_position.hpp`
- `src/sys/update_prev_position.cpp`

Modified:

- `src/core/factories.cpp` — emplace `PrevPosition` on player + every ghost; Blinky overrides it.
- `src/core/game.cpp` — `updatePrevPosition(reg)` as the first call in `Game::logic`.
- `src/sys/movement.cpp` — sync `PrevPosition` on tunnel wrap.
- `src/sys/apply_ghost_speed.cpp` — same tunnel sync in `applyGhostSpeedExtraStep`; `applyGhostSpeedGate` now uses `SlowEffect.timer` parity.
- `src/sys/render.cpp` — `playerRender` and `ghostRender` interpolate `PrevPosition` → `Position`.

## Validation

- Release build via `cmake --build .` from `build/` succeeds (CMake `GLOB_RECURSE` picks up new files automatically).
- Frozen, Slow, and Speed bonuses each render with the expected motion: frozen sits, slow slides at half rate without snap-back, speed slides at double rate without teleport.
