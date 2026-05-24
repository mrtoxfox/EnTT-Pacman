# Study Bugs for EnTT-Pacman — Design and Answer Key

- Date: 2026-05-22
- Status: design approved, pending implementation
- Audience: instructor only

This document is the answer key. It must never reach students or any branch
they receive. See "Delivery model" for where it may live.

## Goal

Add a graded set of 8 bugs to the EnTT-Pacman codebase as a debugging exercise.
Students fix them with an LLM and chain-of-thought reasoning, optionally with a
debugger. Each bug must be non-obvious, cause real wrong behavior, and need
actual investigation rather than a glance at the diff.

## Quality bar

Every bug meets all of these:

- Compiles warning-clean under the project's Debug flags (`-Wall -Wextra -Wpedantic`).
- No crash at startup. The game launches and runs.
- Deterministic. The same play produces the same failure.
- Observable in normal play. The student does not need special input to notice it.
- A one or two line change, confined to a single file.

B10 is a deliberate exception to "no crash". It crashes on a specific in-game
action, not at startup, and runs normally until triggered.

## The set

Eight bugs, picked by the instructor from a 10-bug catalog. The IDs keep the
original catalog numbering, so the set is B1, B2, B3, B4, B6, B7, B9, B10. The
two not selected, B5 and B8, are in the appendix for reference.

Difficulty tiers, for a graded ladder:

- Tier 1, easy: B1, B2, B3
- Tier 2, medium: B4, B6, B7
- Tier 3, hard: B9
- B10: easy to locate from a Debug-build assertion, harder from a Release crash.

## Delivery model

Baseline: commit `890be6d` ("Add audio support with SDL2_mixer and implement
sound effects"), the current `HEAD`. This commit has the finished audio system,
which B3 needs.

Cut every bug branch from commit `890be6d` directly. `master` is one commit
behind, at `cf6e46f`, and does not have the audio system. Branching a bug from
`master` would leave `src/sys/audio.cpp` missing and B3 inapplicable. If
`master` is later fast-forwarded to `890be6d`, this is unchanged: branch from
`890be6d`.

One branch per bug, eight branches, each cut from the baseline:

| Branch | Bug | File |
|--------|-----|------|
| `study-bug/b01-ghost-vibrate` | B1 | `src/sys/pursue_target.cpp` |
| `study-bug/b02-house-exit` | B2 | `src/core/game.cpp` |
| `study-bug/b03-audio-roar` | B3 | `src/sys/audio.cpp` |
| `study-bug/b04-scared-forever` | B4 | `src/sys/change_ghost_mode.cpp` |
| `study-bug/b06-eaten-stuck` | B6 | `src/sys/house.cpp` |
| `study-bug/b07-render-slide` | B7 | `src/sys/render.cpp` |
| `study-bug/b09-collide-edge` | B9 | `src/sys/player_ghost_collide.cpp` |
| `study-bug/b10-energizer-crash` | B10 | `src/sys/change_ghost_mode.cpp` |

Each branch carries exactly one commit: the one or two line change, in one file.
That keeps the branches independently testable and lets the instructor merge any
combination of them.

The instructor reviews each branch and merges the chosen bugs into one combined
branch. This document is the answer key, so it must not sit on any bug branch.
Keep it on a separate branch, on `master` only, or outside git.

## The 8 bugs

Line numbers below are at the baseline commit. The exact before/after text is
the authority if a line number drifts.

### B1 — Ghosts vibrate in place

- Branch: `study-bug/b01-ghost-vibrate`
- File: `src/sys/pursue_target.cpp`, function `pursueTarget`, line 36
- Tier: 1, easy

Before:

```cpp
    for (const Dir candDir : dir_range) {
      // can't go back the way we came
      if (candDir == opposite(dir)) {
        continue;
      }
```

After:

```cpp
    for (const Dir candDir : dir_range) {
      // can't go back the way we came
      if (candDir == dir) {
        continue;
      }
```

Mechanism. The guard exists to stop a ghost reversing into the tile it just
came from. With `dir` instead of `opposite(dir)` it forbids continuing in the
current direction instead. A ghost can reverse but never go straight. In a
straight corridor the only non-wall moves are forward and back, so the ghost
flips 180 degrees every step and bounces between two tiles. The comment "can't
go back the way we came" now contradicts the code.

Symptom. Ghosts twitch one tile back and forth and never travel across the
maze. The player clears the maze almost unopposed.

Debugging path. The symptom points straight at movement, and `pursueTarget` is
the direction-choosing system. The fix needs the student to understand what the
guard is for. The stale comment is the tell.

Verify fixed. Ghosts move continuously and pursue the player.

### B2 — Two ghosts never leave the house

- Branch: `study-bug/b02-house-exit`
- File: `src/core/game.cpp`, function `Game::logic`, lines 93-94
- Tier: 1, easy to medium

Before:

```cpp
  leaveHouse(reg);
  pursueTarget(reg, maze);
```

After:

```cpp
  pursueTarget(reg, maze);
  leaveHouse(reg);
```

Mechanism. `leaveHouse` overwrites a house-bound ghost's `Target` with
`outsideHouse` so that `pursueTarget` steers it to the door. Its own comment
states it must run after the `set*Target` systems. Run `pursueTarget` first and
it reads the un-overwritten target, which is the ghost's scatter corner.
Blinky spawns outside the house already, so it is unaffected. Pinky's scatter
corner is `{0,0}`, above the house, so "move toward the target" still walks it
up and out through the door. Inky (`{18,21}`) and Clyde (`{0,21}`) have scatter
corners at the bottom of the maze. Moving up always increases their distance to
those corners, so the greedy `pursueTarget` never picks up, and they oscillate
inside the house.

Symptom. Inky and Clyde sit in the ghost house the whole game, drifting left
and right. Blinky and Pinky play normally. Two ghosts stuck, not all four. That
asymmetry is the clue.

Debugging path. The bug is in no ghost's code. The student has to notice that
only two ghosts are stuck, look at what those two share, and land on system
order. The `leaveHouse` comment ("called after the set*Target systems") states
the required order.

Verify fixed. All four ghosts leave the house and pursue.

### B3 — Audio degrades into a roar

- Branch: `study-bug/b03-audio-roar`
- File: `src/sys/audio.cpp`, function `audio`, lines 22-24
- Tier: 1, easy

Before:

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

After:

```cpp
  std::vector<entt::entity> played;
  const auto events = reg.view<SoundEvent>();
  for (const entt::entity e : events) {
    device.playSfx(events.get<SoundEvent>(e).id);
    played.push_back(e);
  }
```

Mechanism. `SoundEvent` entities are throwaway. A system creates one each time
it emits a sound, and the `audio` system is meant to destroy them once played.
Drop the destroy loop and they accumulate. Every later `audio` call replays
every `SoundEvent` ever created. The `played` vector is still built and now
never read; this draws no warning because it is a non-trivial type used through
`push_back`. The comment on the loop above still says "then destroy the
throwaway event entities".

Symptom. Within a few seconds the sound effects pile into a constant
overlapping roar. The game gradually slows as the `SoundEvent` entity count
climbs every logic step.

Debugging path. The audio degradation is obvious. The cause needs the student
to see that `SoundEvent`s are entities with a lifecycle. The stale comment and
the now-unused `played` vector both point at the missing cleanup.

Verify fixed. Each sound plays once. No slowdown.

### B4 — Ghosts stay frightened forever

- Branch: `study-bug/b04-scared-forever`
- File: `src/sys/change_ghost_mode.cpp`, function `ghostScaredTimeout`, line 31
- Tier: 2, medium

Before:

```cpp
    ScaredMode &scared = view.get<ScaredMode>(e);
    --scared.timer;
```

After:

```cpp
    ScaredMode scared = view.get<ScaredMode>(e);
    --scared.timer;
```

Mechanism. `view.get<ScaredMode>(e)` returns a reference to the live component.
Binding it to `ScaredMode scared` with no `&` copies it. `--scared.timer`
decrements the copy. The component in the registry never changes, so its timer
never reaches 0 and the `remove<ScaredMode>` branch never runs.

Symptom. Eat one energizer and the ghosts stay frightened and edible for the
rest of the game. They never revert to chase.

Debugging path. Classic reference-versus-value. The line looks correct at a
glance. The student has to know that `view.get` returns a reference and that
dropping the `&` silently copies. A watch on the registry's `ScaredMode.timer`
in a debugger shows it never moving.

Verify fixed. Scared mode ends after `ghostScaredTime` logic steps and ghosts
revert to chase.

### B6 — Eaten ghosts never rejoin the chase

- Branch: `study-bug/b06-eaten-stuck`
- File: `src/sys/house.cpp`, function `enterHouse`, line 25
- Tier: 2, medium

Before:

```cpp
      reg.remove<EnterHouse>(e);
      reg.emplace<LeaveHouse>(e);
      reg.remove<EatenMode>(e);
      reg.emplace<ChaseMode>(e);
```

After:

```cpp
      reg.remove<EnterHouse>(e);
      reg.emplace<LeaveHouse>(e);
      reg.emplace<ChaseMode>(e);
```

Mechanism. An eaten ghost reaches its home tile carrying `EatenMode` and an
`EnterHouse` ticket. `enterHouse` is meant to fully reset it: drop the enter
ticket and `EatenMode`, grant a `LeaveHouse` ticket and `ChaseMode`. Skip
`remove<EatenMode>` and the ghost ends up holding two mode tags at once,
`EatenMode` and `ChaseMode`, which breaks the "four modes are mutually
exclusive" invariant documented in `comp/ghost_mode.hpp`. `setEatenTarget`
still matches the ghost and, running after the chase setters, keeps overwriting
its target with the house. The ghost does leave the house once, because it has
the `LeaveHouse` ticket, but with no `EnterHouse` ticket it cannot pass back
down through the door, so it parks just above the door chasing a target it
cannot reach.

Symptom. Every ghost the player eats is gone from the chase for the rest of the
game. It returns to the house, comes back out, then gets stuck oscillating just
above the door. It renders as a normal ghost, because `ghostRender` checks
`ChaseMode` before `EatenMode`, but the player passes through it harmlessly,
because collision still sees `EatenMode`.

Debugging path. Only shows after eating a ghost, so it is intermittent. The
student traces the eaten ghost's lifecycle, finds `enterHouse`, and has to spot
that one of the four reset steps is missing, leaving the entity with two mode
tags. Knowing the modes are meant to be mutually exclusive is what makes it
click.

Verify fixed. An eaten ghost returns to the house and rejoins the chase as a
normal ghost.

### B7 — Pac-Man slides into walls

- Branch: `study-bug/b07-render-slide`
- File: `src/sys/render.cpp`, function `playerRender`, line 23
- Tier: 2, medium

Before:

```cpp
    const Dir actualDir = view.get<ActualDir>(e).d;
```

After:

```cpp
    const Dir actualDir = view.get<DesiredDir>(e).d;
```

Mechanism. `pos` is the player's tile position in pixels. `toPos(actualDir,
frame)` is the sub-tile offset that slides the sprite from this tile toward the
next as `frame` advances. It must use `ActualDir`, the wall-validated direction
the player is really moving. `DesiredDir` is the direction the input asked for,
which may point into a wall. Rendered with `DesiredDir`, when the player holds
a key against a wall the sprite slides toward the wall each logic step and
snaps back. Logic, collision and dot-eating all read `Position` and `ActualDir`
and are unaffected. The bug is purely visual.

Make the change on the declaration line, not on the `writer.tilePos(...)` call.
If you instead change the call, the `actualDir` variable goes unused and
`-Wunused-variable` fires under `-Wall`. The variable keeps its now-inaccurate
name, which is acceptable for a study bug.

Symptom. Pac-Man visibly slides part-way into a wall when the player holds a
key against one, then snaps back. Nothing else misbehaves. He eats, dies and
moves on the correct tiles.

Debugging path. The student has to separate render from logic. Gameplay is
correct and only the picture is wrong, so the bug is in `render.cpp`. Then they
need to know the difference between `ActualDir` and `DesiredDir` and why the
smooth-motion offset needs the actual one. A good two-rate-loop exercise.

Verify fixed. The sprite never crosses into a wall. It only slides toward tiles
the player actually moves to.

### B9 — Collisions trigger wrong at tile edges

- Branch: `study-bug/b09-collide-edge`
- File: `src/sys/player_ghost_collide.cpp`, function `collide`, line 29
- Tier: 3, hard

Before:

```cpp
  if (pPos == gPos)               return true;
  if (pPos + toPos(pDir) != gPos) return false;
  if (pDir != opposite(gDir))     return false;
  return true;
```

After:

```cpp
  if (pPos == gPos)               return true;
  if (pPos + toPos(pDir) != gPos) return false;
  if (pDir != gDir)               return false;
  return true;
```

Mechanism. `collide` catches two cases: the player and ghost on the same tile,
or on adjacent tiles closing on each other. The adjacency case should require
the two to face opposite ways, `pDir == opposite(gDir)`, meaning they move
toward each other. With `gDir` it fires when they move the same way instead. So
a player one tile behind a ghost, both heading the same direction, registers a
collision they would never actually have. And a genuine head-on approach no
longer registers until the two land on the exact same tile.

Symptom. The player gets killed while trailing a ghost they never catch up to,
and can sometimes slip head-on past a ghost without dying. Rare and situational,
because it depends on the directions lining up, so it is hard to reproduce on
demand.

Debugging path. The student has to read the collision math and work out what
`opposite(gDir)` encodes. A debugger breakpoint in `collide`, printing the
directions at a wrong death, is the fastest route. Hard because it is
intermittent and the code looks plausible.

Verify fixed. Collisions happen on contact, same tile or closing head-on, and
not while following a ghost.

### B10 — Crash on a double energizer

- Branch: `study-bug/b10-energizer-crash`
- File: `src/sys/change_ghost_mode.cpp`, function `ghostScared`, line 20
- Tier: 2 to 3. Easy to locate from a Debug assertion, harder from a Release crash.

Before:

```cpp
    reg.remove_if_exists<ChaseMode, ScatterMode, ScaredMode>(e);
```

After:

```cpp
    reg.remove_if_exists<ChaseMode, ScatterMode>(e);
```

Mechanism. `ghostScared` runs when an energizer is eaten. It clears whatever
mode tag a ghost has, then `emplace<ScaredMode>`. Drop `ScaredMode` from the
`remove_if_exists` list and a ghost that is already scared keeps its
`ScaredMode`. Then `emplace<ScaredMode>` runs on an entity that already has the
component. EnTT 3.4.0 forbids that. In a Debug build `ENTT_ASSERT` fires
immediately with a clear message. In a Release build, where asserts are off, it
corrupts the component pool's sparse set and crashes or misbehaves later.

Trigger. Eat a second energizer while the ghosts are still frightened from the
first.

Symptom. The game aborts (Debug) or crashes (Release) the moment that second
energizer is eaten. It runs fine until then.

Debugging path. A crash gives a stack trace pointing into EnTT's `emplace`. The
key move is building Debug (`-DCMAKE_BUILD_TYPE=Debug`) so the assertion names
the problem precisely. The student then has to see that `ghostScared` can run
on an already-scared ghost, and that the `remove_if_exists` list is meant to
clear every mode tag including the current one.

Verify fixed. Eating a second energizer during fright re-triggers fright with
no crash, and the scared timer resets to full.

## Verifying a fix

Build and run from `build/`:

```
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
./pacman
```

For B10, build Debug instead, so EnTT's assertion fires with a clear message:

```
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
```

A Debug build also turns on `-Wall -Wextra -Wpedantic`, which is the
warning-clean check every bug must pass. Build each branch Debug once to confirm
no new warning.

## Merge notes for the instructor

- The eight bugs touch eight separate concerns. On separate branches they do
  not interact.
- B4 and B10 are both in `src/sys/change_ghost_mode.cpp` but in different
  functions, `ghostScaredTimeout` at line 31 and `ghostScared` at line 20. They
  merge without conflict.
- B10 crashes the game on a double energizer. In a combined branch this blocks
  play-testing every bug that needs play to surface. Merge B10 last, or keep it
  on its own branch.
- B1, B2 and B6 all affect ghost movement. Combined on one branch their
  symptoms overlap and partly mask each other. Acceptable for a single combined
  exercise, but worth knowing when choosing what to merge together.
- Commit messages on the bug branches name the bug. That history is the answer
  key. Squash or reset it before any branch reaches students.

## Appendix — catalog entries not selected

These two were in the 10-bug catalog but not chosen for this set. Kept here in
case the instructor wants them later.

- B5, the maze cannot be won. `src/core/maze.cpp`. Turn one interior `.` in the
  maze string into a space. The maze then has 151 dots while `dotsInMaze` stays
  152, so the `dots == dotsInMaze` win check never trips.
- B8, ghosts wander instead of chasing. `src/sys/set_target.cpp`, function
  `setScaredTarget`. Remove `ScaredMode` from the view's component list. The
  view then matches every ghost, and `setScaredTarget`, running after the chase
  setters, overwrites the chase targets they just computed.
