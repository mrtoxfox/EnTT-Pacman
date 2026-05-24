# Fixes on `audio_backup` vs `master`

Two groups of fixes:

1. **Audio code-review fixes:** issues raised reviewing the audio subsystem added on this branch. Landed in commit `85637e7`.
2. **Study-bug fixes already present here:** six deliberately-injected bugs that exist on `master` (merged from the `study-bug/*` branches) but are not present on `audio_backup`, which was cut from the clean baseline `890be6d` before the bug branches were merged. Each is described with mechanism, symptom, and the exact patch.

---

## 1. Audio code-review fixes

| # | File | Severity | Issue | Fix |
|---|------|----------|-------|-----|
| 1 | `src/core/audio.cpp` | Important | `Audio` constructor threw mid-load, leaking already-loaded chunks/tracks and the open mixer device (destructor doesn't run on partial construction). | Wrapped the load loop in `try { ... } catch (...) { free all loaded resources; Mix_Quit; Mix_CloseAudio; throw; }`. |
| 2 | `src/core/audio.cpp` | Important | No `Mix_Init` / `Mix_Quit` pair. Worked only because all assets are WAV; silently breaks if anyone adds OGG/MP3. | Added `Mix_Init(0)` after `Mix_OpenAudio` and `Mix_Quit()` in the destructor + catch path, with a comment noting the flag mask is 0 because assets are WAV. |
| 3 | `CMakeLists.txt` | Important | `if(CMAKE_BUILD_TYPE MATCHES DEBUG)` is case-sensitive and never matched the conventional value `Debug`, so `-Wall -Wextra -Wpedantic` were never applied. CLAUDE.md's warning-clean promise was silently broken. | Changed to `if(CMAKE_BUILD_TYPE STREQUAL "Debug")`. |
| 4 | `src/core/audio.hpp` | Important | `Audio` indexes into `chunks[]`/`tracks[]` using `SoundId` ordinal position with no compile-time enforcement of the layout. Reordering the enum would silently play the wrong file or go out of bounds. | Added two `static_assert`s: `intro` must immediately follow the last SFX, `loseMusic` must be the last music id. |
| 5 | `src/util/grid.hpp` | Minor (pre-existing, surfaced by #3) | `i >= area()` compared `std::size_t` against `int`, triggering `-Wsign-compare`. | Cast `area()` to `std::size_t` at the call site. Safe because `area()` is always non-negative. |
| 6 | `CMakeLists.txt` | Minor (pre-existing, surfaced by #3) | Generated `sprites.cpp` embeds a vendored tinf inflate impl with an unused `sourceLen` param. CLAUDE.md forbids hand-editing the generated file. | Suppressed `-Wunused-parameter` only for that file in Debug builds via `set_source_files_properties`. |

---

## 2. Study-bug fixes already present in `audio_backup`

The instructor's study-bug exercise injected eight bugs onto branches cut from `890be6d`. Six of those bugs were merged into `master` (the seventh and eighth, **B1** and **B4**, were never merged, so neither branch is affected). `audio_backup` was cut from `890be6d` before the merges happened, so it carries the **correct** code for each of the six. The descriptions below name the file, function, exact lines, what is wrong on `master`, the patch that fixes it (already in place here), and why the patch is correct.

Line numbers reference the file's current state on `master`.

### B2. Two ghosts never leave the house

- **File / function:** `src/core/game.cpp`, `Game::logic`, lines 90-94
- **Branch:** `study-bug/b02-house-exit`

**Buggy code (master):**

```cpp
setScaredTarget(reg, maze, rand);
setScatterTarget(reg);
setEatenTarget(reg);
pursueTarget(reg, maze);
leaveHouse(reg);
```

**Fix (already on this branch):**

```cpp
setScaredTarget(reg, maze, rand);
setScatterTarget(reg);
setEatenTarget(reg);
leaveHouse(reg);
pursueTarget(reg, maze);
```

**Mechanism.** `leaveHouse` rewrites a house-bound ghost's `Target` to `outsideHouse` so the greedy `pursueTarget` steers it toward the door. The contract is documented in `leaveHouse`'s own comment: it must run **after** the `set*Target` systems and **before** `pursueTarget`. With the two swapped, `pursueTarget` reads each ghost's pre-`leaveHouse` target, the scatter corner. Blinky and Pinky are unaffected (Blinky spawns outside the house, Pinky's scatter corner is `{0,0}` so "head toward target" still walks it up and out). Inky and Clyde's scatter corners are at the **bottom** of the maze (`{18,21}` and `{0,21}`), so any upward step strictly increases their straight-line distance and the greedy chooser refuses to take it. The two ghosts oscillate inside the house indefinitely.

**Symptom.** Inky and Clyde sit in the ghost house drifting left and right for the whole game. Blinky and Pinky play normally. The asymmetry between which ghosts get stuck is the diagnostic clue.

**Why the fix is correct.** The original `leaveHouse(reg); pursueTarget(reg, maze);` order is the documented contract. The five lines now appear in the deliberate, load-bearing call sequence that CLAUDE.md flags as "order is load-bearing".

### B3. Audio degrades into a roar

- **File / function:** `src/sys/audio.cpp`, `audio`, around lines 16-24
- **Branch:** `study-bug/b03-audio-roar`

**Buggy code (master):**

```cpp
std::vector<entt::entity> played;
const auto events = reg.view<SoundEvent>();
for (const entt::entity e : events) {
  device.playSfx(events.get<SoundEvent>(e).id);
  played.push_back(e);
}
```

**Fix (already on this branch):**

```cpp
std::vector<entt::entity> played;
const auto events = reg.view<SoundEvent>();
for (const entt::entity e : events) {
  device.playSfx(events.get<SoundEvent>(e).id);
  played.push_back(e);
}
for (const entt::entity e : played) {
  reg.destroy(e);
}
```

**Mechanism.** `SoundEvent` is documented as a transient throwaway entity: a system emits one, the `audio` system plays it once and destroys it. Drop the destroy loop and every event accumulates in the registry forever. The next tick re-plays every `SoundEvent` ever created, plus any newly emitted ones. Within a few seconds the SFX channels stack into a constant overlapping roar; the entity count rises every logic step and the game slows. The `played` vector is still populated, just never consumed, which doesn't draw a warning because it is a non-trivial type used through `push_back`, and the comment above the loop still says "then destroy the throwaway event entities", which is the dead giveaway.

**Symptom.** SFX pile into a constant roar within seconds; framerate degrades over time as the registry grows.

**Why the fix is correct.** Restores the lifecycle contract every other ticket-style component in the codebase follows (`EnterHouse`, `LeaveHouse`): the system that acts on a ticket is responsible for removing it. Two-pass destruction (collect into `played`, then `destroy`) is safe even though EnTT 3.4.0 permits mutating the current view entity, because here we are destroying *every* entity yielded by the same view we are iterating; the two-pass form is documented as the conservative idiom.

### B6. Eaten ghosts never rejoin the chase

- **File / function:** `src/sys/house.cpp`, `enterHouse`, lines 22-26
- **Branch:** `study-bug/b06-eaten-stuck`

**Buggy code (master):**

```cpp
reg.remove<EnterHouse>(e);
reg.emplace<LeaveHouse>(e);
reg.emplace<ChaseMode>(e);
```

**Fix (already on this branch):**

```cpp
reg.remove<EnterHouse>(e);
reg.emplace<LeaveHouse>(e);
reg.remove<EatenMode>(e);
reg.emplace<ChaseMode>(e);
```

**Mechanism.** An eaten ghost reaches its home tile carrying an `EatenMode` tag and an `EnterHouse` ticket. The reset has four steps: drop the enter ticket, grant a `LeaveHouse` ticket, drop `EatenMode`, add `ChaseMode`. Skip the third step and the ghost ends up wearing **two** mode tags at once, `EatenMode` and `ChaseMode`. This breaks the invariant called out explicitly in `comp/ghost_mode.hpp`: the four ghost modes are mutually exclusive (this is precisely why they are modeled as separate tag components rather than an enum). `setEatenTarget`, which matches `<Ghost, EatenMode>`, still matches the ghost and keeps overwriting its target with the house. The ghost leaves once (`LeaveHouse` ticket exists), reaches the door, then cannot pass back down (no `EnterHouse` ticket) and parks just above the door endlessly chasing the house tile it cannot reach.

**Symptom.** Every ghost the player eats vanishes from the chase for the rest of the game. It returns to the house, comes back out, then oscillates just above the door. It renders as a normal ghost because `ghostRender` checks `ChaseMode` before `EatenMode`, but the player passes through it harmlessly because collision still sees `EatenMode`.

**Why the fix is correct.** Restores the mutual-exclusion invariant on ghost-mode tags. After the four lines run, the ghost holds exactly one mode tag (`ChaseMode`) and one ticket (`LeaveHouse`), which is the expected state for a ghost about to exit the house.

### B7. Pac-Man slides into walls

- **File / function:** `src/sys/render.cpp`, `playerRender`, line 23
- **Branch:** `study-bug/b07-render-slide`

**Buggy code (master):**

```cpp
const Dir actualDir = view.get<DesiredDir>(e).d;
```

**Fix (already on this branch):**

```cpp
const Dir actualDir = view.get<ActualDir>(e).d;
```

**Mechanism.** `Position` is integer tile coordinates; smooth motion is rendered by adding `toPos(actualDir, frame)`, a sub-tile pixel offset, to the on-screen quad position. That offset must use the **wall-validated** direction `ActualDir`, which is the direction the player is actually moving. `DesiredDir` is the raw input intent, which may point straight into a wall. Reading the offset from `DesiredDir` means: when the player holds a key against a wall, each logic step the sprite slides part-way **into** the wall as `frame` advances, then snaps back when the next logic step lands and `frame` wraps. Logic, collision, and dot-eating all read `Position` and `ActualDir`, so they are unaffected; the bug is purely visual.

The fix has to be made on the variable's declaration, not at the `writer.tilePos(...)` call site. If you instead correct the call, the now-unused `actualDir` variable trips `-Wunused-variable` under the Debug warning flags.

**Symptom.** When the player holds a movement key against a wall, the sprite visibly slides part-way into the wall and snaps back. Gameplay (eating, dying, moving on grid) remains correct.

**Why the fix is correct.** Restores the two-rate-loop contract: integer-tile logic + sub-tile interpolation in render must agree on which direction the entity is actually moving.

### B9. Collisions trigger wrong at tile edges

- **File / function:** `src/sys/player_ghost_collide.cpp`, `collide`, line 29
- **Branch:** `study-bug/b09-collide-edge`

**Buggy code (master):**

```cpp
if (pPos == gPos)               return true;
if (pPos + toPos(pDir) != gPos) return false;
if (pDir != gDir)               return false;
return true;
```

**Fix (already on this branch):**

```cpp
if (pPos == gPos)               return true;
if (pPos + toPos(pDir) != gPos) return false;
if (pDir != opposite(gDir))     return false;
return true;
```

**Mechanism.** `collide` catches two cases: the player and ghost on the same tile, or on adjacent tiles closing head-on. The first two lines handle the same-tile case and the adjacency check. The third line should enforce that the two are facing **opposite** directions, i.e. closing on each other (`pDir == opposite(gDir)`). With `pDir != gDir` the third check accepts the case where both move the **same** direction. So:

- A player one tile behind a ghost, both heading the same way, registers a collision they would never actually have (the player is *chasing* the ghost, not closing on it). The player dies.
- A genuine head-on approach (player going right, ghost going left, ghost one tile to the right) is now **rejected** by the third check; no collision is reported until the two land on the same tile, by which point the player has slid through the ghost during the sub-tile interpolation.

**Symptom.** The player gets killed while trailing a ghost from behind without catching up, and can sometimes slip head-on past a ghost without dying. Both modes are rare because they depend on the directions lining up exactly, which makes the bug hard to reproduce on demand.

**Why the fix is correct.** The adjacency check is only meaningful for closing collisions. `pDir == opposite(gDir)` is the canonical "moving toward each other" predicate; the `!=` form skips that case.

### B10. Corrupts state / Crash on a double energizer

- **File / function:** `src/sys/change_ghost_mode.cpp`, `ghostScared`, line 20
- **Branch:** `study-bug/b10-energizer-crash`

**Buggy code (master):**

```cpp
void ghostScared(entt::registry &reg) {
  const auto view = reg.view<Ghost>();
  for (const entt::entity e : view) {
    reg.remove_if_exists<ChaseMode, ScatterMode>(e);
    if (!reg.has<EatenMode>(e)) {
      reg.emplace<ScaredMode>(e);
    }
  }
}
```

**Fix (already on this branch):**

```cpp
void ghostScared(entt::registry &reg) {
  const auto view = reg.view<Ghost>();
  for (const entt::entity e : view) {
    reg.remove_if_exists<ChaseMode, ScatterMode, ScaredMode>(e);
    if (!reg.has<EatenMode>(e)) {
      reg.emplace<ScaredMode>(e);
    }
  }
}
```

**Mechanism.** `ghostScared` runs on every energizer eat. It is supposed to drop **whatever** mode tag a ghost currently wears, then add `ScaredMode`. With `ScaredMode` missing from the `remove_if_exists` list, a ghost that is already scared (because the player ate a previous energizer recently) keeps its `ScaredMode`. The next line, `reg.emplace<ScaredMode>(e)`, then runs on an entity that already has the component. EnTT 3.4.0's contract for `emplace` is that the component must not already exist on the entity.

- In a Debug build, `ENTT_ASSERT` fires immediately with a clear message naming `ScaredMode`.
- In a Release build, the assertion is compiled out. The component pool's sparse set is left in an inconsistent state and the process either crashes shortly after or misbehaves in subtle ways (the same sparse index now points to two pool slots).

**Trigger.** Eat a second energizer while the ghosts are still frightened from the first. Easy to reproduce; energizers are common.

**Symptom.** The game aborts (Debug) or crashes / corrupts state (Release) the moment the second energizer is eaten. The game runs fine until that moment.

**Why the fix is correct.** Restores the "clear every mode tag, including the current one" invariant the function relies on. After the `remove_if_exists`, the ghost provably has no mode tag, so the subsequent `emplace<ScaredMode>` is safe regardless of prior state.

---

## Verification

```
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
./pacman
```

Builds warning-clean on `audio_backup`. None of the six study-bug symptoms reproduces: ghosts leave the house and pursue, SFX play once and the audio system stays quiet between events, eaten ghosts rejoin the chase, Pac-Man's sprite never slides into a wall, collisions fire on contact head-on but not while trailing, and eating a second energizer while ghosts are scared neither asserts nor crashes.
