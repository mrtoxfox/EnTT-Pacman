//
//  sound_event.hpp
//  EnTT Pacman
//

#ifndef COMP_SOUND_EVENT_HPP
#define COMP_SOUND_EVENT_HPP

#include "core/sound_id.hpp"

// A request to play a sound effect. A system that detects an event creates a
// throwaway entity carrying this component (much like the EnterHouse and
// LeaveHouse "ticket" tags). The audio system plays the sound and destroys the
// event entity, so these never live longer than one logic step.
struct SoundEvent {
  SoundId id;
};

#endif
