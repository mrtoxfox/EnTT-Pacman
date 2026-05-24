# Universal Bonus System — Design

A maze-spawned pickup system for the EnTT Pac-Man tutorial. Bonuses appear on
walkable tiles, the player collects them by walking over them, and each kind
applies a timed effect to every ghost: Freeze (no movement), Slow (half rate),
or Speed (double rate). A sound fires on spawn, on collection, and on expiry.

The design follows the codebase's existing ECS conventions: plain-data
components, free-function systems with one job each, mutually-exclusive tag
components for state, the throwaway-entity pattern for one-shot events, and a
documented order in `Game::logic`.

## 1. Existing pieces this hooks into

- **Two-rate loop**. `app.cpp` runs at `fps = 30` frames per second; `Game::logic`
  runs once every `tileSize = 8` frames (so one logic tick is `8/30 ≈ 0.267 s`).
  `Position` is integer tile coords; render extrapolates sub-tile motion forward
  along `ActualDir`. Speed/slow have to be expressed as whole-tile steps per
  tick — fractional velocity does not fit the model.
- **Tag-with-timer pattern**. `ScaredMode` (`comp/ghost_mode.hpp`) and
  `ImmortalMode` (`comp/immortal_mode.hpp`) carry a countdown. A `*Timeout`
  system decrements and removes them. The three new ghost-effect tags use the
  same shape.
- **Mutually exclusive tags**. The four ghost-mode tags are switched via
  `remove_if_exists` + `emplace` (`sys/change_ghost_mode.cpp`). The three new
  effect tags follow the same idiom.
- **One-shot tickets / throwaway events**. `EnterHouse`, `LeaveHouse`, and the
  `SoundEvent` entity demonstrate "create, consume, destroy in the same tick"
  components. Bonus pickup events are modeled as `SoundEvent` entities.
- **Maze tile state**. `MazeState = Grid<Tile>` lives outside the ECS and is the
  authority for what's walkable. The spawn system reads it.
- **Audio**. `SoundId` lists 5 SFX then 5 music ids. `audio.cpp` loads each id's
  file. New ids are added at the end of the SFX block; `sfxCount` updates
  automatically because it's derived from `SoundId::intro`'s value.
- **Rendering geometry without sprites**. `hudRender` and `pauseOverlayRender`
  draw rectangles through `SDL_Renderer` directly, sidestepping the Animera-
  generated sprite sheet. Bonus rendering uses the same path.

## 2. Components

All in `src/comp/`, plain aggregates, brace-initialized.

### `comp/bonus.hpp`

```cpp
enum class BonusKind : std::uint8_t { freeze, slow, speed };

struct Bonus {
  BonusKind kind;
  int lifeTimer;  // logic ticks until despawn if uncollected
};
```

A bonus on the maze is one entity carrying `Bonus` and `Position`. No effect on
gameplay until the player walks over it.

### `comp/bonus_spawner.hpp`

```cpp
struct BonusSpawner {
  int timer;  // logic ticks until the next spawn attempt
};
```

One entity holds this, created in `Game::init`. The component lives in the ECS
rather than on `Game`, matching the existing reasoning in `game.cpp` that ECS
state generalizes more cleanly (e.g. multi-player support).

### `comp/ghost_speed_effect.hpp`

Three mutually exclusive tag-with-timer types:

```cpp
struct FrozenEffect { int timer; };
struct SlowEffect   { int timer; };
struct SpeedEffect  { int timer; };
```

Applying an effect to a ghost removes any of the other two first
(`remove_if_exists<FrozenEffect, SlowEffect, SpeedEffect>`), then `emplace`s the
new one with a fresh timer. Same lifecycle as `ScaredMode`.

### `comp/no_move.hpp`

```cpp
struct NoMove {};
```

A per-tick gate: an entity with this tag is skipped by `movement`. Re-issued
fresh each tick, never persisted across ticks. Single source of truth, see
the system order below.

## 3. Systems

All in `src/sys/`, header + impl, first parameter `entt::registry &`.

### `sys/spawn_bonus.{hpp,cpp}` — `spawnBonus(reg, maze, rand)`

- Decrement `BonusSpawner::timer`.
- On zero, and only if no `Bonus` entity exists already (one-at-a-time):
  - Sample `(x, y)` uniformly in the maze where:
    - `maze[{x,y}] != Tile::wall` and `!= Tile::door`,
    - `y < 8 || y > 12` (excludes the ghost-house rows and the tunnel band,
      matching the special-cased coordinates in `sys/movement.cpp` and the
      coordinates in `core/constants.hpp`),
    - no ghost or player currently occupies that tile.
  - Roll a `BonusKind` uniformly from the three.
  - `reg.create()`; `emplace<Position>`, `emplace<Bonus>` with
    `lifeTimer = bonusLifeTicks`.
  - Emit `SoundEvent { SoundId::bonusSpawn }`.
- Reset `BonusSpawner::timer` to a fresh uniform value in
  `[bonusSpawnMinTicks, bonusSpawnMaxTicks]` whether or not a spawn occurred
  (a failed spawn just retries on the next window).

### `sys/bonus_timeout.{hpp,cpp}` — `bonusTimeout(reg)`

- Decrement `Bonus::lifeTimer` for each bonus entity.
- Destroy the entity on zero. Silent expiry (no sound).

### `sys/eat_bonus.{hpp,cpp}` — `eatBonus(reg)`

- Iterate `Player, Position` × `Bonus, Position`. On position match:
  - For every `Ghost`:
    - `reg.remove_if_exists<FrozenEffect, SlowEffect, SpeedEffect>(e)`,
    - `reg.emplace<X>(e, bonusEffectTicks)` where `X` matches the picked kind.
  - Emit one `SoundEvent { SoundId::bonusApplied }`.
  - `reg.destroy(bonus)`.

### `sys/ghost_speed_timeout.{hpp,cpp}` — `ghostSpeedTimeout(reg)`

Mirrors `ghostScaredTimeout`. Three near-identical loops (one per effect tag).
Each:

- Decrement the timer.
- On zero, remove the tag.
- Track a single per-call boolean "any effect expired this tick" to dedupe the
  expiry sound; emit `SoundEvent { SoundId::bonusExpired }` once at the end of
  the function if it fired.

### `sys/apply_ghost_speed.{hpp,cpp}`

Two free functions:

#### `applyGhostSpeedGate(reg, tick)`

Runs at the very end of each `Game::logic` tick. Re-issues `NoMove` tags from
scratch using only the EnTT 3.4.0 surface listed in `CLAUDE.md`:

- Iterate `reg.view<NoMove>()` and call `reg.remove<NoMove>(e)` for each entity
  (3.4.0 doesn't expose a typed `clear<T>`; the iterate-and-remove pattern is
  what the rest of the codebase uses).
- For each ghost with `FrozenEffect`: `emplace<NoMove>`.
- For each ghost with `SlowEffect`, on odd `tick % 2`: `emplace<NoMove>`.
- `SpeedEffect` does **not** get `NoMove`; instead its extra step is handled
  by the post-movement system below.

Re-issuing every tick keeps `NoMove`'s lifetime fully owned by this system.
There is no separate "clear" step elsewhere.

#### `applyGhostSpeedExtraStep(reg, maze)`

Runs immediately after `movement` and `wallCollide`. For each ghost with
`SpeedEffect`:

- Recompute `wallCollide` semantics locally for that one ghost and, if it can
  continue in `ActualDir`, advance `Position` by `toPos(ActualDir)`.
- Apply the tunnel wrap exactly as `movement` does at `y == 10`. The four
  lines are duplicated here rather than extracted into a helper: two call
  sites in a tutorial codebase doesn't justify a new utility, and the wrap
  rule already has a documented "hard-coded coordinate" caveat in
  `movement.cpp`.

A ghost with `SpeedEffect` therefore moves 2 tiles per logic tick. Visually
this stutters rather than appearing as continuous fast motion, because the
render code interpolates exactly 1 tile of sub-tile motion per tick. This is a
known trade-off for keeping the two-rate loop intact, and it reads as
"fast and twitchy" — arcade-appropriate.

### `sys/bonus_render.{hpp,cpp}` — `bonusRender(renderer, reg)`

Iterates `Bonus, Position`. For each, fill a colored rectangle inset slightly
from the tile bounds (e.g. 2 px inset on each side) using
`SDL_SetRenderDrawColor` + `SDL_RenderFillRect`, with the color picked by
`BonusKind`:

- Freeze: light cyan
- Slow: green
- Speed: red/orange

Drawn between `dotRender` and `playerRender` so bonuses sit on top of dots but
below sprites. This avoids any change to the Animera-generated sprite sheet.

A simple "about to despawn" flash can be added later by checking `lifeTimer`
parity in the last second; not in v1.

## 4. Movement: the one existing-code edit

In `sys/movement.cpp`, change the view from

```cpp
auto view = reg.view<Position, ActualDir>();
```

to

```cpp
auto view = reg.view<Position, ActualDir>(entt::exclude<NoMove>);
```

That's the only modification to existing systems. `wallCollide` is left
unchanged: it runs for all entities, but ghosts with `NoMove` haven't moved
this tick so wall-collision results are the same as the previous tick (no
harm, and avoids touching another file).

## 5. Constants

Added to `core/constants.hpp`, as `constexpr` at namespace scope:

```cpp
// Bonus tuning. Ticks; the existing `(N * fps) / tileSize` idiom converts
// real-time seconds to logic ticks.
constexpr int bonusEffectTicks   = (10 * fps) / tileSize;   // 37
constexpr int bonusLifeTicks     = (8  * fps) / tileSize;   // 30
constexpr int bonusSpawnMinTicks = (8  * fps) / tileSize;   // 30
constexpr int bonusSpawnMaxTicks = (15 * fps) / tileSize;   // 56
```

`bonusEffectTicks` matches the spec's 10 seconds. The others are reasonable
defaults — adjust freely; they live in the same file as `scatterTicks` etc.

## 6. Sound integration

Three new entries appended to the SFX block of `SoundId`:

```cpp
enum class SoundId : std::uint8_t {
  chomp,
  energizer,
  eatGhost,
  death,
  win,
  bonusSpawn,     // NEW
  bonusApplied,   // NEW
  bonusExpired,   // NEW
  intro,
  background,
  siren,
  winMusic,
  loseMusic,
};
```

Because `sfxCount` in `audio.hpp` is computed as
`static_cast<std::size_t>(SoundId::intro)`, the SFX array size updates
automatically. The music block stays untouched.

In `audio.cpp`'s constructor, the three new chunks are loaded with file paths.
**Asset choice:** reuse existing `.wav` files rather than adding new ones,
matching the existing repurposing pattern documented in `sound_id.hpp`:

- `bonusSpawn`   → `audio/sfx/pacman_eatfruit.wav`
- `bonusApplied` → `audio/sfx/pacman_eatghost.wav`
- `bonusExpired` → `audio/sfx/pacman_chomp.wav`

The user can drop in dedicated files later by changing the load paths only.

## 7. `Game::logic` order

Updated sequence with new calls marked. Order is load-bearing; reasoning is in
the comments to the right.

```
movement(reg);                       // skips NoMove-tagged entities
applyGhostSpeedExtraStep(reg, maze); // NEW: SpeedEffect ghosts take a 2nd tile
wallCollide(reg, maze);
dots += eatDots(reg, maze);
if (eatEnergizer(reg, maze)) { ghostScared(reg); }

ghostScaredTimeout(reg);
ghostSpeedTimeout(reg);              // NEW: tick down Freeze/Slow/Speed timers
immortalTimeout(reg);
bonusTimeout(reg);                   // NEW: despawn uncollected bonuses

enterHouse(reg);
setBlinkyChaseTarget(reg);
setPinkyChaseTarget(reg);
setInkyChaseTarget(reg);
setClydeChaseTarget(reg);
setScaredTarget(reg, maze, rand);
setScatterTarget(reg);
setEatenTarget(reg);
leaveHouse(reg);
immortalOverride(reg);
pursueTarget(reg, maze);

// Collision / win check (unchanged)
const GhostCollision collision = playerGhostCollide(reg);
if (collision.type == GhostCollision::Type::eat) { ghostEaten(reg, collision.ghost); }
if (collision.type == GhostCollision::Type::lose) { ... }
else if (dots == dotsInMaze) { state = State::won; }

spawnBonus(reg, maze, rand);         // NEW: maybe spawn a bonus
eatBonus(reg);                       // NEW: player-bonus pickup
applyGhostSpeedGate(reg, ticks);     // NEW: re-issue NoMove for next tick

audio(reg, device);                  // unchanged; consumes all SoundEvents
// win/lose music block (unchanged)
```

Placement rationale:

- `applyGhostSpeedExtraStep` follows `movement` directly so it sees the just-
  applied tile step and can decide whether a second one fits.
- `ghostSpeedTimeout` is grouped with other timeouts.
- `bonusTimeout` is grouped with other timeouts.
- `spawnBonus` and `eatBonus` come after the win-check block so they don't
  interfere with the dot count, but before `audio` so their `SoundEvent`s are
  flushed this tick.
- `applyGhostSpeedGate` is the last logic call before `audio`: it issues the
  `NoMove` tags that `movement` will read on the next tick.

## 8. `Game::init` additions

- After `makeMazeState()`, create the spawner entity:
  ```cpp
  const entt::entity spawner = reg.create();
  reg.emplace<BonusSpawner>(spawner, bonusSpawnMinTicks);
  ```

## 9. `Game::render` additions

Add a single call between `dotRender` and `playerRender` inside the
`State::playing` / `State::paused` branch:

```cpp
dotRender(writer, maze);
bonusRender(renderer, reg);          // NEW
playerRender(reg, writer, renderFrame);
ghostRender(reg, writer, renderFrame);
```

No changes to win/lose render paths.

## 10. Files touched

**New components** (`src/comp/`):

- `bonus.hpp`
- `bonus_spawner.hpp`
- `ghost_speed_effect.hpp`
- `no_move.hpp`

**New systems** (`src/sys/`):

- `spawn_bonus.{hpp,cpp}`
- `bonus_timeout.{hpp,cpp}`
- `eat_bonus.{hpp,cpp}`
- `ghost_speed_timeout.{hpp,cpp}`
- `apply_ghost_speed.{hpp,cpp}`
- `bonus_render.{hpp,cpp}`

**Edited**:

- `src/sys/movement.cpp` — add `entt::exclude<NoMove>` to the view
- `src/core/sound_id.hpp` — three new SFX ids
- `src/core/audio.cpp` — load three new chunks (reusing existing `.wav` files)
- `src/core/constants.hpp` — four new constants
- `src/core/game.cpp` — wire new systems into `init`, `logic`, `render`
- `src/core/game.hpp` — no change (registry holds the new entities)

`CMakeLists.txt` does not need editing: `file(GLOB_RECURSE ... CONFIGURE_DEPENDS)`
picks up the new files automatically.

## 11. Build order (incremental, each step playable)

1. **Constants + sound ids + chunk loads.** Compile and run. No behaviour change.
2. **`NoMove` tag + `movement` view exclusion.** Still no behaviour change
   because nothing emits the tag.
3. **`FrozenEffect` + `ghostSpeedTimeout` + `applyGhostSpeedGate`**, wired into
   `Game::logic`. Temporarily add `reg.emplace<FrozenEffect>(blinky, 200)` in
   `Game::init` to verify a frozen ghost stops, then unfreezes. Remove the
   temp line.
4. **`SlowEffect`** through the same gate. Verify the slowed ghost moves every
   other tick.
5. **`SpeedEffect` + `applyGhostSpeedExtraStep`**. Verify the sped-up ghost
   moves two tiles per tick.
6. **`Bonus` + `BonusSpawner` + `spawnBonus` + `bonusTimeout` + `bonusRender`**.
   Bonuses appear, sit, despawn. No effect yet.
7. **`eatBonus`** wires collection to effects. Full feature works.
8. **Audio polish.** Verify spawn / applied / expired sounds fire as expected
   and the expiry dedupe is correct.

## 12. Known trade-offs (recorded so they don't surprise a reader)

- **Speed ghosts stutter.** Two-tile-per-tick motion does not interpolate
  smoothly under the existing render model; the visual is choppy. Acceptable
  for this style of game and keeps the loop intact. The alternative
  (modifying the render rate or sub-tick motion) would touch the entire game
  loop and is rejected as out of scope.
- **Slow ghosts also stutter.** Half-rate movement renders as a step-pause-
  step pattern rather than continuous half-speed motion, for the same reason.
- **Bonus expiry sound fires once per call, not once per ghost.** The
  dedupe is in `ghostSpeedTimeout`. If multiple effects expire in the same
  tick (rare), only one sound plays. Acceptable.
- **Reused SFX assets.** Three new `SoundId`s point at existing `.wav` files
  for now. Replacing them is a one-line change in `audio.cpp` per id.
- **One bonus on the maze at a time.** Keeps the rules simple and gives each
  effect more weight. Easy to lift later by removing the early-out in
  `spawnBonus`.
- **Effect replacement is total.** A new pickup replaces any active effect on
  every ghost. There is no stacking, no per-ghost variation.
