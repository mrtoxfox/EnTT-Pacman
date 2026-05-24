//
//  bonus_spawner.hpp
//  EnTT Pacman
//

#ifndef COMP_BONUS_SPAWNER_HPP
#define COMP_BONUS_SPAWNER_HPP

// Singleton that holds the countdown to the next bonus spawn attempt. Created
// in Game::init and ticked by spawnBonus. Lives in the ECS rather than on the
// Game class so that all transient game state stays in one place.
struct BonusSpawner {
  int timer;
};

#endif
