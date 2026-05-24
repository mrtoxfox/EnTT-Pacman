//
//  audio.hpp
//  EnTT Pacman
//

#ifndef CORE_AUDIO_HPP
#define CORE_AUDIO_HPP

#include <array>
#include <cstddef>
#include <SDL_mixer.h>
#include "sound_id.hpp"

// Owns the audio device and every loaded sound. Construction opens the device
// and loads the assets from the audio folder; destruction frees them. Sound
// effects can overlap on separate channels. Only one music track plays at a
// time, so starting a track stops the one before it.
class Audio {
public:
  Audio();
  ~Audio();

  Audio(const Audio &) = delete;
  Audio &operator=(const Audio &) = delete;

  // Plays a one-shot sound effect on the first free channel
  void playSfx(SoundId);
  // Starts a music track, looping it forever when loop is true
  void playMusic(SoundId, bool loop);

  // The track passed to the most recent playMusic call
  SoundId currentMusic() const;
  // Whether a music track is currently audible
  bool musicPlaying() const;

private:
  // The sound-effect ids are the ones before SoundId::intro
  static constexpr std::size_t sfxCount =
    static_cast<std::size_t>(SoundId::intro);
  static constexpr std::size_t musicCount =
    static_cast<std::size_t>(SoundId::loseMusic)
    - static_cast<std::size_t>(SoundId::intro) + 1;

  std::array<Mix_Chunk *, sfxCount> chunks{};
  std::array<Mix_Music *, musicCount> tracks{};
  SoundId current = SoundId::intro;
};

#endif
