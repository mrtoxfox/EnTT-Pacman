//
//  spawn_bonus.hpp
//  EnTT Pacman
//

#ifndef SYS_SPAWN_BONUS_HPP
#define SYS_SPAWN_BONUS_HPP

#include <random>
#include "core/maze.hpp"
#include <entt/entity/fwd.hpp>

// Ticks the BonusSpawner timer down. On zero, attempts to spawn one bonus on
// a random walkable tile (no walls, doors, ghost-house rows, or occupied
// tiles) and emits a bonusSpawn SoundEvent. Skipped while a bonus is already
// on the maze. Resets the spawner timer to a random value in
// [bonusSpawnMinTicks, bonusSpawnMaxTicks] regardless of outcome.
void spawnBonus(entt::registry &, const MazeState &, std::mt19937 &);

#endif
