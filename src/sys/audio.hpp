//
//  audio.hpp
//  EnTT Pacman
//

#ifndef SYS_AUDIO_HPP
#define SYS_AUDIO_HPP

#include <entt/entity/fwd.hpp>

class Audio;

// Plays every queued SoundEvent and destroys the event entities. Also keeps
// the music in sync with play: the frightened siren loops while any ghost is
// scared, otherwise the background track loops. The intro jingle is left to
// finish on its own before any looping music starts.
void audio(entt::registry &, Audio &);

#endif
