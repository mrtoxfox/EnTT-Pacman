//
//  sound_id.hpp
//  EnTT Pacman
//

#ifndef CORE_SOUND_ID_HPP
#define CORE_SOUND_ID_HPP

#include <cstdint>

// Identifies one sound. The sound-effect ids come first and are loaded as
// short, overlapping Mix_Chunks. The music ids come after and are loaded as
// Mix_Music tracks; only one music track plays at a time.
//
// The game has no fruit, extra-life or intermission feature, so a few of the
// bundled sounds are repurposed: pacman_eatfruit plays on an energizer,
// pacman_extrapac on a win, pacman_intermission as the win-screen music and
// pacman_ringtone_interlude as the lose-screen music. The three bonus ids
// also reuse existing chunks (see audio.cpp); they can point at dedicated
// files later without touching anything else.
enum class SoundId : std::uint8_t {
  // Sound effects
  chomp,         // pacman eats a dot
  energizer,     // pacman eats an energizer
  eatGhost,      // pacman eats a frightened ghost
  death,         // pacman is caught by a ghost
  win,           // the player clears the maze
  bonusSpawn,    // a bonus appears on the maze
  bonusApplied,  // the player picks up a bonus
  bonusExpired,  // a bonus effect runs out

  // Music (every id from here on is a music track)
  intro,       // start jingle, played once
  background,  // looping music during normal play
  siren,       // looping music while any ghost is frightened
  winMusic,    // music on the win screen
  loseMusic,   // music on the lose screen
};

#endif
