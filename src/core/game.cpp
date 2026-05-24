//
//  game.cpp
//  EnTT Pacman
//
//  Created by Indiana Kernick on 22/9/18.
//  Copyright © 2018 Indiana Kernick. All rights reserved.
//

#include "game.hpp"

#include "audio.hpp"
#include "constants.hpp"
#include "sys/house.hpp"
#include "sys/audio.hpp"
#include "comp/score.hpp"
#include "comp/lives.hpp"
#include "comp/player.hpp"
#include "sys/render.hpp"
#include "sys/eat_dots.hpp"
#include "sys/movement.hpp"
#include "core/factories.hpp"
#include "sys/set_target.hpp"
#include "sys/eat_bonus.hpp"
#include "sys/spawn_bonus.hpp"
#include "comp/sound_event.hpp"
#include "sys/player_input.hpp"
#include "sys/bonus_render.hpp"
#include "sys/bonus_timeout.hpp"
#include "comp/immortal_mode.hpp"
#include "sys/pursue_target.hpp"
#include "comp/bonus_spawner.hpp"
#include "sys/immortal_timeout.hpp"
#include "sys/immortal_override.hpp"
#include "sys/change_ghost_mode.hpp"
#include "sys/apply_ghost_speed.hpp"
#include "sys/ghost_speed_timeout.hpp"
#include "sys/player_ghost_collide.hpp"
#include "sys/update_prev_position.hpp"

void Game::init(Audio &device) {
  maze = makeMazeState();
  const entt::entity player = makePlayer(reg);
  const entt::entity blinky = makeBlinky(reg, player);
  makePinky(reg, player);
  makeInky(reg, player, blinky);
  makeClyde(reg, player);
  // Singleton holding the next-bonus countdown. Lives in the ECS so all
  // transient game state stays in one place.
  reg.emplace<BonusSpawner>(reg.create(), bonusSpawnMinTicks);
  // seeding a pseudo random number generator with a random source
  rand.seed(std::random_device{}());
  // The intro jingle plays once; the audio system starts the looping
  // background music after it finishes
  device.playMusic(SoundId::intro, false);
}

bool Game::input(Audio &device, const SDL_Scancode key) {
  if (key == SDL_SCANCODE_ESCAPE) {
    return false;
  }
  if (key == SDL_SCANCODE_SPACE) {
    if (state == State::playing) {
      state = State::paused;
      device.pauseAll();
    } else if (state == State::paused) {
      state = State::playing;
      device.resumeAll();
    }
    return true;
  }
  if (key == SDL_SCANCODE_P) {
    if (state == State::playing) {
      state = State::pausedDebug;
      device.pauseAll();
    } else if (state == State::pausedDebug) {
      state = State::playing;
      device.resumeAll();
    }
    return true;
  }
  if (state == State::playing) {
    playerInput(reg, maze, key);
  }
  return true;
}

bool Game::logic(Audio &device) {
  // The order of systems is very important in an ECS. Each system reads some
  // state and modifies some state. If the state isn't read and modified in the
  // right order, subtle bugs can occur. Make sure that the order of systems is
  // easy to see (i.e. not hidden away by some abstraction that sets the order
  // for you). Always think carefully about the order that systems should be in.

  // It's OK to keep some game state outside of the ECS (e.g. maze, dots,
  // dotSprite) but try to keep as much state within the ECS as you can.
  // Keeping too much state outside of the ECS can lead to problems.
  // For example: `dots` is the amount of dots eaten by the player. If there
  // were more than one player, then each player might want to keep track of how
  // many dots they've eaten. So `dots` would have to be moved into a component

  if (state != State::playing) {
    return true;
  }

  if (scattering) {
    if (ticks >= scatterTicks) {
      ghostChase(reg);
      ticks = 0;
      scattering = false;
    }
  } else {
    if (ticks >= chaseTicks) {
      ghostScatter(reg);
      ticks = 0;
      scattering = true;
    }
  }
  ++ticks;

  // Snapshot start-of-tick positions before any system mutates Position.
  // Render interpolates PrevPosition -> Position over the next tileSize
  // frames, which is what makes Speed and Slow effects look smooth.
  updatePrevPosition(reg);

  movement(reg);
  applyGhostSpeedExtraStep(reg, maze);
  wallCollide(reg, maze);
  dots += eatDots(reg, maze);
  if (eatEnergizer(reg, maze)) {
    ghostScared(reg);
  }
  ghostScaredTimeout(reg);
  ghostSpeedTimeout(reg);
  immortalTimeout(reg);
  bonusTimeout(reg);
  enterHouse(reg);
  setBlinkyChaseTarget(reg);
  setPinkyChaseTarget(reg);
  setInkyChaseTarget(reg);
  setClydeChaseTarget(reg);
  setScaredTarget(reg, maze, rand);
  setScatterTarget(reg);
  setEatenTarget(reg);
  leaveHouse(reg);
  immortalOverride(reg);
  pursueTarget(reg, maze);

  const GhostCollision collision = playerGhostCollide(reg);
  if (collision.type == GhostCollision::Type::eat) {
    ghostEaten(reg, collision.ghost);
  }
  if (collision.type == GhostCollision::Type::lose) {
    // Find the player that was caught. There's only one player entity, but
    // iterate the view so we stay ECS-shaped.
    const auto players = reg.view<Player, Lives, Score>();
    for (const entt::entity p : players) {
      Lives &lives = players.get<Lives>(p);
      Score &score = players.get<Score>(p);
      --lives.remaining;
      score.value /= 2;
      if (lives.remaining <= 0) {
        state = State::lost;
      } else {
        reg.emplace<ImmortalMode>(p);
        // Queue the death SFX as a regular sound event so it plays via the
        // audio system on this same tick.
        reg.emplace<SoundEvent>(reg.create(), SoundId::death);
      }
    }
  } else if (dots == dotsInMaze) {
    state = State::won;
  }

  // Bonuses run after the win check so they don't interfere with the dot
  // count, but before audio so their SoundEvents are flushed this tick.
  spawnBonus(reg, maze, rand);
  eatBonus(reg);
  // applyGhostSpeedGate is the last logic call: it issues the NoMove tags
  // that movement() will read on the next tick.
  applyGhostSpeedGate(reg);

  // Play the sounds queued by the systems above and update the music. The
  // win and lose sounds are one-shots tied to the end of the game, so they
  // are played here directly. Skip the audio system on a state transition so
  // its music-management loop doesn't briefly restart background/siren before
  // the end-screen track takes over (would surface as a flash of the wrong
  // music on lose).
  if (state == State::playing) {
    audio(reg, device);
  } else if (state == State::lost) {
    device.playSfx(SoundId::death);
    device.playMusic(SoundId::loseMusic, true);
  } else if (state == State::won) {
    device.playSfx(SoundId::win);
    device.playMusic(SoundId::winMusic, true);
  }

  return true;
}

void Game::render(SDL_Renderer *renderer, SDL::QuadWriter &writer, const int frame) {
  if (state == State::playing || state == State::paused || state == State::pausedDebug) {
    // While playing, cache the sub-tile frame each render. On pause, reuse the
    // last cached value so the freeze keeps the pixel position the player saw
    // at press time. Snapping to 0 would jump the sprite back to the tile
    // boundary - invisible under the dim overlay but obvious for pausedDebug.
    if (state == State::playing) {
      frozenFrame = frame;
    }
    const int renderFrame = (state == State::playing) ? frame : frozenFrame;
    fullRender(writer, animera::SpriteID::maze);
    dotRender(writer, maze);
    bonusRender(renderer, reg);
    playerRender(reg, writer, renderFrame);
    ghostRender(reg, writer, renderFrame);
    if (state == State::paused) {
      pauseOverlayRender(renderer);
      pauseTextRender(renderer);
    }
    hudRender(renderer, writer, reg);
  } else if (state == State::won) {
    fullRender(writer, animera::SpriteID::win);
    summaryRender(renderer, writer, reg);
  } else if (state == State::lost) {
    fullRender(writer, animera::SpriteID::lose);
    summaryRender(renderer, writer, reg);
  }
}
