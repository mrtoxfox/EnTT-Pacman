# EnTT Pacman

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](./LICENSE)

An example of how to use the ECS part of the [EnTT Framework](https://github.com/skypjack/entt).

## The ECS approach

EnTT is an entity-component-system library. The idea is to split a game into
three plain pieces:

- **Entities** are just IDs. A ghost, the player, a wall - each is one ID with
  no behavior of its own.
- **Components** are plain data attached to an entity: a position, a sprite, a
  direction. No methods, no logic.
- **Systems** are free functions that run over every entity holding a given set
  of components and do one job - move them, draw them, check collisions.

So instead of a `Ghost` class that inherits from `Actor` and owns its movement,
collision and rendering code, a ghost is an ID with `Position`, `Sprite` and
`Ghost` components, and separate systems handle each concern. Behavior comes
from which components an entity has, not from a class hierarchy. To change what
an entity does, you add or remove components rather than picking a different
subclass.

This project leans into that. Ghost mode, for example, is four tag components
instead of an enum, and a ghost switches mode by swapping one tag for another.
A single `entt::registry` (owned by `Game`) holds every entity and component.
The whole game loop is one fixed sequence of system calls. If that sounds
unfamiliar, reading the source alongside this README is the point - the code is
small and each system does one thing.

## Sound

Sound follows the same ECS approach. A system detecting an event (a dot eaten,
a ghost eaten) creates a throwaway entity with a `SoundEvent` component; the
`audio` system plays the queued sounds and manages the music. The looping
background track gives way to a siren while ghosts are frightened. Assets live
in `audio/`, split into `sfx/` and `music/`.

The game has no fruit, extra-life or intermission feature, so a few of the
bundled arcade sounds are reused: the fruit sound plays on an energizer, the
extra-life jingle on a win, and the intermission and interlude tracks become
the win and lose screen music.

## Installing SDL2 and SDL2_mixer

This uses the [SDL2 Library](https://www.libsdl.org/) for input and rendering,
and [SDL2_mixer](https://github.com/libsdl-org/SDL_mixer) for sound. CMake will
find both if they're on your system. For details on how to install SDL2, see
the [installation page](https://wiki.libsdl.org/Installation).

If you're on MacOS,

```
brew install sdl2 sdl2_mixer
```

If you're on Ubuntu (or another Debian based system),

```
sudo apt-get install libsdl2-dev libsdl2-mixer-dev
```

If you're on RHEL (or another Red Hat based system),

```
sudo dnf install SDL2-devel SDL2_mixer-devel
```

If you're on Windows,

```
vcpkg install sdl2 sdl2-mixer
```

## Building

[EnTT](https://github.com/skypjack/entt) is bundled with the project to make
building this as easy as possible. You need CMake 3.12 or newer (CMake 4.x
works too).

### macOS and Linux

On macOS, install the Xcode command line tools (`xcode-select --install`) to
get Clang. On Linux you need a C++17 compiler, CMake and Make. On Ubuntu use
`sudo apt-get install build-essential cmake`, and on RHEL
`sudo dnf install gcc-c++ cmake make`.

```
git clone https://github.com/indianakernick/EnTT-Pacman.git
cd EnTT-Pacman/build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
./pacman
```

### Windows

Install SDL2 and SDL2_mixer with vcpkg first (see *Installing SDL2 and
SDL2_mixer* above), then point CMake at the vcpkg toolchain file so it can find
the libraries. Replace `C:/vcpkg` with wherever you cloned vcpkg.

```
git clone https://github.com/indianakernick/EnTT-Pacman.git
cd EnTT-Pacman\build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
Release\pacman.exe
```

The Visual Studio generator is multi-config: the build type is chosen at build
time with `--config`, and the executable lands in a `Release\` subfolder. If
the SDL2 DLLs aren't already on your `PATH`, copy `SDL2.dll` and
`SDL2_mixer.dll` from vcpkg (`C:/vcpkg/installed/x64-windows/bin`) next to
`pacman.exe` before running it. The build copies the `audio` folder next to the
executable automatically, so the game finds its sounds wherever it's launched.

## Controls

- WASD or arrow keys: move Pac-Man
- SPACE: pause (dim overlay, "PAUSED" text, audio paused)
- P: debug pause (freezes the frame without any overlay, audio paused) - useful for inspecting a single tick
- ESC: quit

## It's not exactly the same as the real thing

I read [The Pacman Dossier](http://tralvex.com/download/forum/The%20Pac-Man%20Dossier.pdf)
many times during development. If you notice a difference between this game and
the real thing, that wasn't an oversight. Perfectly recreating the real thing
would have made this project quite a bit more complicated. I think it's
complicated enough as it is!
