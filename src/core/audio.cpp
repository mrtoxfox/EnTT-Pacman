//
//  audio.cpp
//  EnTT Pacman
//

#include "audio.hpp"

#include <string>
#include <SDL.h>
#include "constants.hpp"
#include "util/sdl_check.hpp"

namespace {

struct SoundFile {
  SoundId id;
  const char *path;
};

// Where each sound is loaded from. Sound effects live in audio/sfx, music
// tracks in audio/music. Paths are relative to the executable's directory.
constexpr SoundFile soundFiles[] = {
  {SoundId::chomp,      "audio/sfx/pacman_chomp.wav"},
  {SoundId::energizer,  "audio/sfx/pacman_eatfruit.wav"},
  {SoundId::eatGhost,   "audio/sfx/pacman_eatghost.wav"},
  {SoundId::death,      "audio/sfx/pacman_death.wav"},
  {SoundId::win,        "audio/sfx/pacman_extrapac.wav"},
  {SoundId::intro,      "audio/music/pacman_beginning.wav"},
  {SoundId::background, "audio/music/pacman_background.wav"},
  {SoundId::siren,      "audio/music/pacman_ringtone.wav"},
  {SoundId::winMusic,   "audio/music/pacman_intermission.wav"},
  {SoundId::loseMusic,  "audio/music/pacman_ringtone_interlude.wav"},
};

bool isMusic(const SoundId id) {
  return static_cast<int>(id) >= static_cast<int>(SoundId::intro);
}

std::size_t sfxIndex(const SoundId id) {
  return static_cast<std::size_t>(id);
}

std::size_t musicIndex(const SoundId id) {
  return static_cast<std::size_t>(id)
    - static_cast<std::size_t>(SoundId::intro);
}

// The directory the executable lives in. Asset paths are resolved relative to
// this so the game can be launched from any working directory.
std::string basePath() {
  char *path = SDL_GetBasePath();
  std::string result = path ? path : "";
  SDL_free(path);
  return result;
}

}

Audio::Audio() {
  SDL_CHECK(Mix_OpenAudio(
    audioFrequency, MIX_DEFAULT_FORMAT, audioOutputChannels, audioChunkSize
  ));
  // All bundled assets are .wav, which Mix_LoadWAV/Mix_LoadMUS decode without
  // any optional decoder. The Mix_Init(0)/Mix_Quit() pair is still kept so
  // adding an .ogg/.mp3 only needs the flag mask changed here.
  Mix_Init(0);
  try {
    const std::string base = basePath();
    for (const SoundFile &file : soundFiles) {
      const std::string full = base + file.path;
      if (isMusic(file.id)) {
        tracks[musicIndex(file.id)] = SDL_CHECK(Mix_LoadMUS(full.c_str()));
      } else {
        chunks[sfxIndex(file.id)] = SDL_CHECK(Mix_LoadWAV(full.c_str()));
      }
    }
  } catch (...) {
    for (Mix_Chunk *chunk : chunks) {
      Mix_FreeChunk(chunk);
    }
    for (Mix_Music *track : tracks) {
      Mix_FreeMusic(track);
    }
    Mix_Quit();
    Mix_CloseAudio();
    throw;
  }
}

Audio::~Audio() {
  Mix_HaltMusic();
  Mix_HaltChannel(-1);
  for (Mix_Chunk *chunk : chunks) {
    Mix_FreeChunk(chunk);
  }
  for (Mix_Music *track : tracks) {
    Mix_FreeMusic(track);
  }
  Mix_Quit();
  Mix_CloseAudio();
}

void Audio::playSfx(const SoundId id) {
  Mix_PlayChannel(-1, chunks[sfxIndex(id)], 0);
}

void Audio::playMusic(const SoundId id, const bool loop) {
  Mix_PlayMusic(tracks[musicIndex(id)], loop ? -1 : 1);
  current = id;
}

void Audio::pauseAll() {
  Mix_PauseMusic();
  Mix_Pause(-1);
}

void Audio::resumeAll() {
  Mix_ResumeMusic();
  Mix_Resume(-1);
}

SoundId Audio::currentMusic() const {
  return current;
}

bool Audio::musicPlaying() const {
  return Mix_PlayingMusic() != 0;
}
