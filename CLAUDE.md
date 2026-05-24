# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

A Pac-Man clone in C++17, written as a tutorial for the ECS part of the EnTT framework. SDL2 handles input and rendering, SDL2_mixer handles sound. EnTT 3.4.0 is bundled in `third_party/entt`. Deviations from arcade Pac-Man are deliberate simplifications, not bugs (see the README).

## Build and run

CMake out-of-source build. The `build/` directory is committed (its contents are gitignored except `.gitignore`) and is where you build:

```
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
./pacman
```

- CMake 3.12 or newer is required. `cmake_minimum_required` uses the `3.12...4.2` range syntax, so the project also builds cleanly (no policy warnings) on CMake 4.x.
- SDL2 and SDL2_mixer must be installed on the system (`brew install sdl2 sdl2_mixer`, `apt-get install libsdl2-dev libsdl2-mixer-dev`, or `vcpkg install sdl2 sdl2-mixer`). CMake finds them via `cmake/modules/FindSDL2.cmake` and `FindSDL2_mixer.cmake`, or via a vcpkg toolchain file when `-DCMAKE_TOOLCHAIN_FILE=...` is passed (see `appveyor.yml`).
- The build copies `audio/` next to the executable with a `POST_BUILD` step in `CMakeLists.txt`. The game loads its sounds at runtime from a path relative to `SDL_GetBasePath()`, so they are not embedded in the executable (unlike the sprite texture).
- Debug builds (`-DCMAKE_BUILD_TYPE=Debug`) add `-Wall -Wextra -Wpedantic`.
- No test suite and no linter. CI (`.travis.yml`, `appveyor.yml`) only builds Release.
- `CMakeLists.txt` builds the source list with `file(GLOB_RECURSE ... CONFIGURE_DEPENDS)` over `src/*.cpp` and `src/*.hpp`. New files are picked up automatically on the next build, no edit to `CMakeLists.txt` needed (the `.hpp` glob is just for IDE tooling).

Controls: WASD or arrow keys to move. SPACE pauses (dim overlay + "PAUSED" text + audio paused). P is a debug pause that only freezes the frame (no overlay, no text). ESC quits. `Game::input` returns `false` to signal quit; `Application::run` watches the return value.

## Architecture

Entity-Component-System built on EnTT. A single `entt::registry` (owned by `Game`) holds all entities and components.

### Entry path

`main.cpp` -> `Application` (`core/app.cpp`) sets up SDL, the window, the renderer, loads the texture, and runs the main loop. `Game` (`core/game.cpp`) owns the registry, the maze, and the win/lose state.

### Two-rate game loop (key concept)

The loop in `Application::run()` runs at `fps` (30, in `constants.hpp`) and splits work across two rates:

- `Game::logic(Audio &)` runs once every `tileSize` (8) frames. One logic step equals one tile of movement.
- `Game::render()` runs every frame. It receives `frame % tileSize`, the sub-tile pixel offset, so sprites slide smoothly between tile positions.

`fps` is the game-speed lever: logic and render rates scale with it together, so raising it speeds up the whole game while keeping motion smooth. It was raised from 20 to 30. Don't repurpose `tileSize` for speed; it is also the tile pixel size and the render interpolation divisor.

So an entity's `Position` is always integer tile coordinates. The smooth motion is purely a render-time effect. Understanding this means reading `app.cpp`, `game.cpp`, and `constants.hpp` together.

### Game states

`Game::State` is `playing | paused | pausedDebug | won | lost`. `Game::logic` early-returns on anything but `playing`, so all four non-playing states freeze the world. The two pause variants differ only in render: `paused` draws the dim overlay and "PAUSED" text (`pauseOverlayRender` + `pauseTextRender` in `sys/render.cpp`); `pausedDebug` skips both, leaving the unmodified frame so you can inspect a single tick. Both pause audio via `Audio::pauseAll`. `Game::input` returns `bool` (`false` on ESC) so `Application::run` can quit the main loop.

### Systems

Systems are free functions in `src/sys/`, each a header/impl pair, taking `entt::registry &` (sometimes plus the maze or RNG). `Game::logic()` calls them in a fixed, deliberate order. That order is load-bearing: each system reads and writes shared component state, and reordering them causes subtle bugs. The comment in `game.cpp` says this explicitly. When adding a system, place its call carefully in that sequence.

### Components

Plain structs in `src/comp/`, many of them empty tag types. Ghost mode is modeled as four mutually exclusive tag components (`ChaseMode`, `ScatterMode`, `ScaredMode`, `EatenMode`) instead of an enum, so per-mode data can live on the tag (`ScaredMode` carries a countdown timer). Changing mode means removing one tag and adding another. `EnterHouse`/`LeaveHouse` act as "tickets" that grant a ghost permission to pass through the house door.

### Ghost AI

Each ghost is created in `core/factories.cpp` with a distinct `*ChaseTarget` component (`comp/chase_target.hpp`). A chase target stores entity IDs (e.g. the player), so a ghost chases whatever entity is referenced rather than hard-coded behavior.

- `sys/set_target.cpp` computes each ghost's target tile: one system per ghost for chase mode, plus scatter, scared, and eaten variants.
- `sys/pursue_target.cpp` picks the direction that greedily minimizes straight-line distance to the target. No pathfinding ("Pacman doesn't use A*"), which matches the original game.
- `Game` alternates global scatter and chase phases using `ticks` against `scatterTicks`/`chaseTicks`.

### Maze and state kept outside the ECS

The maze is a `Grid<Tile>` (`core/maze.hpp`, `util/grid.hpp`) built from an ASCII string literal in `maze.cpp`. Dots and energizers are tiles in that grid, not entities. `eatDots` mutates the grid directly. `dotsInMaze` (152, in `constants.hpp`) is the win condition and must match the dot count in the maze string. Keeping the maze and dot count outside the ECS is a deliberate call, explained in a comment in `game.cpp`.

### Hard-coded world assumptions

The tunnel (wrap-around at row y=10) and the ghost-house door orientation are special-cased with literal coordinates in `sys/movement.cpp` and `sys/can_move.cpp`. Comments flag this as a deliberate shortcut over modeling them as entities. Maze, spawn, and ghost home/scatter coordinates live in `core/constants.hpp`.

### Generated sprite code

`src/util/sprites.hpp` and `sprites.cpp` are generated by the Animera tool from the `.animera` files in `animations/` and the config in `sprites.json`. Don't hand-edit them. The texture is embedded in the executable as a deflate-compressed byte array. `SpriteID` is an enum with `_beg_`/`_end_` markers and arithmetic operators, so animation frames can be addressed as `base + frame`.

### Sound

Sound follows the ECS pattern. `Audio` (`core/audio.hpp`) is an RAII wrapper that owns the SDL2_mixer device and every loaded sound; it is constructed in `Application::run()` and passed by reference into `Game::init` and `Game::logic`, the same way `MazeState` and the RNG are passed to systems.

- `SoundEvent` (`comp/sound_event.hpp`) is a transient component carrying a `SoundId`. A system that detects an event creates a throwaway entity with it (`reg.emplace<SoundEvent>(reg.create(), SoundId::chomp)`), the way the `EnterHouse`/`LeaveHouse` ticket tags work. `eatDots`, `eatEnergizer`, and `ghostEaten` emit these.
- `sys/audio.cpp` is the consuming system, the last call in `Game::logic()`. It plays every queued `SoundEvent`, destroys the event entities, and swaps the looping music (background vs frightened siren) based on whether any ghost has `ScaredMode`.
- Music is one global track, so it doesn't map cleanly to per-entity events. The intro jingle is started directly by `Game::init`; the win/lose music and one-shot win/death SFX are played directly by `Game::logic`, mutually exclusive with the audio system. On the tick state transitions to `won` or `lost`, `Game::logic` skips calling `audio()` and plays the end-screen SFX/music directly. Without that skip the audio system's music-management loop briefly restarts background/siren (since neither matches `wanted`), producing a flash of the wrong track before the end music takes over.
- `SoundId` (`core/sound_id.hpp`) lists the five SFX ids first, then the five music ids. Assets live in `audio/sfx/` and `audio/music/`, loaded as `Mix_Chunk` and `Mix_Music` respectively.
- The game has no fruit, extra-life or intermission feature, so four bundled sounds are repurposed (energizer, win jingle, win music, lose music). This mapping is documented in `sound_id.hpp`.

## Conventions when changing code

EnTT's model is: components are plain data, systems are stateless free functions, nothing is registered, and the user owns the loop. This codebase follows that exactly. Match it.

### C++17

- The standard is C++17, set with `target_compile_features(pacman PRIVATE cxx_std_17)` in `CMakeLists.txt`. Don't add a manual `-std=` flag and don't use C++20-or-later features (concepts, ranges, `<format>`, designated initializers, `std::span`, etc.).
- Compile-time constants are namespace-scope `constexpr` in `core/constants.hpp` (`constexpr` gives internal linkage, so it's safe in a header). Add new tunable values there as `constexpr`, never as `#define`.
- File-local helper functions and types go in an anonymous `namespace { }` inside the `.cpp` (see `factories.cpp`, `eat_dots.cpp`). Don't use file-scope `static`.
- Enums are `enum class` with an explicit underlying type (`enum class Dir : std::uint8_t`, `util/dir.hpp`).
- Components and small value types (`Pos`) are aggregates: no constructors, initialize with braces.
- The ghost-mode state deliberately avoids `std::variant` in favor of tag components (the reasoning is a comment in `comp/ghost_mode.hpp`). Don't "modernize" that into a variant or enum.
- Debug builds compile with `-Wall -Wextra -Wpedantic`. Keep new code warning-clean under those.

### Systems

- One system is one free function in `src/sys/`, with a matching header/impl pair. No system classes.
- Systems are stateless. They keep nothing between calls. Persistent state lives in components (or, by exception, on `Game`). Pass extra inputs (`MazeState`, `std::mt19937`) as explicit parameters, never as globals or statics.
- A system's first parameter is `entt::registry &`.
- A new system needs one edit beyond its own files: add the call to the `Game::logic()` sequence in `game.cpp` at the right point in the order. The `.cpp`/`.hpp` are picked up by the `file(GLOB_RECURSE ...)` in `CMakeLists.txt` automatically.
- The order of system calls in `Game::logic()` is load-bearing. Read the surrounding calls before inserting one, and don't reorder existing calls without checking what state each reads and writes.

### Iterating entities

- Iterate with a view: `auto view = reg.view<A, B>(); for (const entt::entity e : view) { view.get<A>(e); }`.
- Use `reg.get<T>(other)` to reach a component on an entity not in the current view (e.g. a ghost system reading the player's `Position`).
- Adding or removing components on the entity currently yielded by the view is safe and is used deliberately (see `sys/change_ghost_mode.cpp`). Changing the view's component set for *other* entities mid-iteration is not safe.

### Components

- Plain structs in `src/comp/`, header-only, one concept per file. No methods, no hand-written constructors. Construct via `reg.emplace<T>(e, fields...)` using aggregate initialization.
- Empty structs are tag components. Prefer a set of mutually exclusive tags over an enum when per-state data differs (see the four ghost-mode tags; `ScaredMode` carries a timer). Switch state by `remove` + `emplace`, not by mutating a field.
- "Ticket" tags (`EnterHouse`/`LeaveHouse`) grant one-shot permission. The system that acts on the ticket removes it.
- Default to keeping state in a component. State lives outside the ECS only with a documented reason (the maze grid and dot count are the existing exceptions, explained in `game.cpp`).

### EnTT 3.4.0 API

The bundled EnTT is 3.4.0. Use only that version's surface: `registry::create`, `emplace<T>`, `get<T>`, `has<T>`, `remove<T>`, `remove_if_exists<T>`, `view<...>` with `view.get<T>(e)`. Later releases renamed and changed these (`has` → `all_of`/`any_of`, `remove_if_exists` removed, `view.get` overloads changed). Don't introduce newer API. Don't upgrade `third_party/entt` without updating every call site.
