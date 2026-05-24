//
//  spawn_bonus.cpp
//  EnTT Pacman
//

#include "spawn_bonus.hpp"

#include "comp/bonus.hpp"
#include "comp/ghost.hpp"
#include "comp/player.hpp"
#include "comp/position.hpp"
#include "core/constants.hpp"
#include "comp/sound_event.hpp"
#include "comp/bonus_spawner.hpp"
#include <entt/entity/registry.hpp>

namespace {

bool tileOccupied(entt::registry &reg, const Pos pos) {
  // Don't place a bonus directly on top of the player or a ghost. Iterating
  // both groups is cheap (1 + 4 entities) and keeps this self-contained.
  const auto players = reg.view<Player, Position>();
  for (const entt::entity e : players) {
    if (players.get<Position>(e).p == pos) return true;
  }
  const auto ghosts = reg.view<Ghost, Position>();
  for (const entt::entity e : ghosts) {
    if (ghosts.get<Position>(e).p == pos) return true;
  }
  return false;
}

bool validSpawnTile(const MazeState &maze, const Pos pos) {
  if (maze.outOfRange(pos)) return false;
  const Tile t = maze[pos];
  if (t == Tile::wall || t == Tile::door) return false;
  // Rows 8..12 cover the ghost-house band and the tunnel row. Bonus placement
  // there leads to awkward pickups behind the door or in the wrap-around, so
  // restrict to the open play area.
  if (pos.y >= 8 && pos.y <= 12) return false;
  return true;
}

int resetTimer(std::mt19937 &rand) {
  std::uniform_int_distribution<int> dist(bonusSpawnMinTicks, bonusSpawnMaxTicks);
  return dist(rand);
}

}

void spawnBonus(entt::registry &reg, const MazeState &maze, std::mt19937 &rand) {
  auto spawners = reg.view<BonusSpawner>();
  for (const entt::entity s : spawners) {
    BonusSpawner &spawner = spawners.get<BonusSpawner>(s);
    if (--spawner.timer > 0) {
      continue;
    }
    // Always roll a fresh interval, even if we skip the spawn below. This
    // means "8-15s after the previous spawn attempt", not "8-15s after the
    // previous bonus expired" - a deliberate choice so the spawner cadence
    // is independent of how long the player takes to pick a bonus up.
    spawner.timer = resetTimer(rand);

    // One bonus on the maze at a time; if one already exists, just bide time.
    if (!reg.view<Bonus>().empty()) {
      continue;
    }

    // Try a bounded number of random tiles before giving up for this round.
    // The play area has plenty of valid tiles; 32 attempts is overkill.
    std::uniform_int_distribution<int> xDist(0, maze.width() - 1);
    std::uniform_int_distribution<int> yDist(0, maze.height() - 1);
    std::uniform_int_distribution<int> kindDist(
      0, static_cast<int>(BonusKind::speed)
    );
    for (int attempt = 0; attempt != 32; ++attempt) {
      const Pos pos{xDist(rand), yDist(rand)};
      if (!validSpawnTile(maze, pos)) continue;
      if (tileOccupied(reg, pos)) continue;

      const entt::entity bonus = reg.create();
      reg.emplace<Position>(bonus, pos);
      reg.emplace<Bonus>(
        bonus,
        static_cast<BonusKind>(kindDist(rand)),
        bonusLifeTicks
      );
      reg.emplace<SoundEvent>(reg.create(), SoundId::bonusSpawn);
      break;
    }
  }
}
