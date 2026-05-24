//
//  audio.cpp
//  EnTT Pacman
//

#include "audio.hpp"

#include <vector>
#include "core/audio.hpp"
#include "comp/ghost_mode.hpp"
#include "comp/sound_event.hpp"
#include <entt/entity/registry.hpp>

void audio(entt::registry &reg, Audio &device) {
  // Play every queued sound effect, then destroy the throwaway event entities
  std::vector<entt::entity> played;
  const auto events = reg.view<SoundEvent>();
  for (const entt::entity e : events) {
    device.playSfx(events.get<SoundEvent>(e).id);
    played.push_back(e);
  }

  // The frightened siren loops while any ghost is scared, the background
  // track loops otherwise. While the intro jingle is still playing, neither
  // takes over; once it finishes, the poll below starts the looping music.
  const bool scared = !reg.view<ScaredMode>().empty();
  const SoundId wanted = scared ? SoundId::siren : SoundId::background;
  const bool introUnfinished =
    device.currentMusic() == SoundId::intro && device.musicPlaying();
  if (!introUnfinished) {
    if (!device.musicPlaying() || device.currentMusic() != wanted) {
      device.playMusic(wanted, true);
    }
  }
}
