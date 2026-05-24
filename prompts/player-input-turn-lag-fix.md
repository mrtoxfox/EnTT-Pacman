# Player input turn lag fix

## Symptom

Pac-Man reacts to direction changes too late and can miss turns at junctions, even though the sprite has already rotated to face the new direction by the time the player sees the turn fail. Pressing a turn key as Pac-Man visually approaches a junction reliably overshoots: the sprite faces the new direction for the rest of the slide, but the next movement step continues in the old direction past the junction.

## Root cause

Two-rate loop: `Game::logic` runs once every `tileSize` (8) frames, render runs every frame and interpolates `PrevPosition` → `Position`. The render uses `DesiredDir` for sprite rotation (`src/sys/render.cpp`), so the sprite turns the instant the key is pressed. Movement uses `ActualDir`.

The system order inside `Game::logic` is:

```cpp
updatePrevPosition(reg);
movement(reg);                 // uses CURRENT ActualDir
applyGhostSpeedExtraStep(reg, maze);
wallCollide(reg, maze);        // updates ActualDir from DesiredDir at the NEW position
```

`wallCollide` runs **after** `movement`. The decision to honor a new `DesiredDir` is therefore made at the post-movement tile, which means it applies to the *next* tick rather than the current one. Concrete timeline with Pac-Man moving right, junction at `(6,10)`, player wants to turn up:

1. End of tick `K-1`: `movement` already advanced `Position` to `(6,10)`. `wallCollide` checked `DesiredDir = right` (unchanged) and left `ActualDir = right`.
2. Frames `(K-1)*tileSize + 1` … `K*tileSize - 1`: render interpolates from `(5,10)` to `(6,10)`. Player sees Pac-Man approaching the junction.
3. Player presses UP mid-slide. `playerInput` sets `DesiredDir = up`. The sprite rotates immediately because `playerRender` reads `DesiredDir`.
4. Tick `K`: `movement` uses the still-unchanged `ActualDir = right` → `Position = (7,10)`. The junction is gone. `wallCollide` then evaluates `DesiredDir = up` at `(7,10)`, finds a wall, and leaves `ActualDir = right`. Turn missed.

The `DesiredDir` buffer only works when the key press lands before the tick that *arrives* at the junction. Presses that land during the visual approach (i.e. between two logic ticks) are always one tile too late.

This is player-specific. Ghosts' `pursueTarget` is intentionally structured to compute the turn for `nextPos = pos + actualDir`, so the post-movement `wallCollide` evaluates the decision at the correct tile for them.

## Fix

Update `ActualDir` directly inside `playerInput`, gated by `canMove` at the player's current tile. `DesiredDir` is still set unconditionally so mid-corridor presses continue to buffer through the existing `wallCollide` path.

### Signature change

`src/sys/player_input.hpp` — `playerInput` now takes the maze so it can call `canMove`:

```cpp
bool playerInput(entt::registry &, const MazeState &, SDL_Scancode);
```

### Input system

`src/sys/player_input.cpp` — after setting `DesiredDir`, attempt an immediate `ActualDir` write:

```cpp
auto view = reg.view<Player, Position, ActualDir, DesiredDir>();
for (const entt::entity e : view) {
  view.get<DesiredDir>(e).d = dir;
  if (canMove(reg, maze, e, view.get<Position>(e).p, dir)) {
    view.get<ActualDir>(e).d = dir;
  }
}
```

### Caller

`src/core/game.cpp` — pass the maze through:

```cpp
playerInput(reg, maze, key);
```

## Why this preserves invariants

`movement` does not check walls — it just adds `toPos(dir)`. The invariant the rest of the codebase relies on is: **`ActualDir` is only ever set to a direction walkable from the current tile**. `wallCollide` was the sole writer enforcing this. The fix preserves the invariant because every new write is gated by the same `canMove(reg, maze, e, pos, dir)` check.

`Position` only changes inside `Game::logic`. Input runs between ticks, so the tile that `canMove` validates against in `playerInput` is the same tile `movement` will start from on the next tick. No TOCTOU between input and movement.

## Side effects audited

- **Ghost AI**: untouched. The view filters by `Player`, so ghost components are not read or written.
- **Tunnel wrap**: between ticks `Position.x` can be `-1` or `19`. `canMove` already handles out-of-range correctly (left/right allowed, perpendicular rejected), so no special case is needed.
- **Pause / won / lost**: `Game::input` already gates `playerInput` behind `state == State::playing`.
- **ImmortalMode / NoMove**: neither alters this path. `NoMove` is ghost-only; `ImmortalMode` affects render alpha and collision skip only.
- **Multiple key presses in one frame**: SDL queues every `SDL_KEYDOWN`; last press wins, same as before.
- **Reversal (pressing opposite direction)**: now instant. Previously delayed by one tick. This matches original Pac-Man behavior and is the same one-tick lag as the missed-turn symptom — fixing one fixes both. A consequence is that the player can dodge a head-on ghost collision by reversing at the last frame; `playerGhostCollide` correctly reflects this because it reads the just-updated `ActualDir`.
- **Mid-corridor turn press**: `canMove` returns false, only `DesiredDir` updates, identical to the old buffering behavior.

## Files touched

Modified:

- `src/sys/player_input.hpp` — signature takes `const MazeState &`; comment expanded to describe the immediate `ActualDir` update.
- `src/sys/player_input.cpp` — write `ActualDir` when the pressed direction is walkable from `Position`.
- `src/core/game.cpp` — pass `maze` to `playerInput`.

## Validation

- `cmake --build .` from `build/` succeeds.
- Press a turn key at any frame during the inter-tick slide toward a junction: the next movement step turns at the junction instead of overshooting.
- Mid-corridor presses still wait for the next valid junction (buffered through `DesiredDir`).
- Ghosts behave identically (verified by code path: the view filter excludes them entirely).
