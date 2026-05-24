//
//  flashlight_render.cpp
//  EnTT Pacman
//

#include "flashlight_render.hpp"

#include <cmath>
#include <vector>
#include <limits>
#include <cstdint>
#include "comp/dir.hpp"
#include "comp/position.hpp"
#include "comp/flashlight.hpp"
#include "core/constants.hpp"
#include "util/dir_to_pos.hpp"
#include "util/sdl_check.hpp"
#include <entt/entity/registry.hpp>

namespace {

constexpr float pi_f = 3.14159265358979323846f;

float facingRad(const Dir d) {
  // Screen coords: +x right, +y down. Matches the 90-deg rotation convention
  // used by playerRender (DesiredDir * 90 degrees).
  switch (d) {
    case Dir::up:    return -pi_f * 0.5f;
    case Dir::right: return  0.0f;
    case Dir::down:  return  pi_f * 0.5f;
    case Dir::left:  return  pi_f;
    default:         return  pi_f;
  }
}

// Walks a ray from (ax, ay) in pixel space along the unit vector (dx, dy)
// until it hits an opaque tile (wall or door) or travels `maxPx` pixels or
// exits the maze. Returns the endpoint in pixel space and whether the stop
// was caused by a wall hit (true) vs the length cap or boundary (false).
// Uses Amanatides & Woo's fast voxel traversal on the tile grid.
bool walkRay(
  const float ax, const float ay,
  const float dx, const float dy,
  const float maxPx,
  const MazeState &maze,
  float &outX, float &outY
) {
  const float eps = 1e-6f;
  const float fx = ax / static_cast<float>(tileSize);
  const float fy = ay / static_cast<float>(tileSize);
  int tx = static_cast<int>(std::floor(fx));
  int ty = static_cast<int>(std::floor(fy));

  const float inf = std::numeric_limits<float>::infinity();

  int stepX;
  float tMaxX, tDeltaX;
  if (dx > eps) {
    stepX = 1;
    tMaxX = (static_cast<float>(tx + 1) - fx) * static_cast<float>(tileSize) / dx;
    tDeltaX = static_cast<float>(tileSize) / dx;
  } else if (dx < -eps) {
    stepX = -1;
    tMaxX = (fx - static_cast<float>(tx)) * static_cast<float>(tileSize) / -dx;
    tDeltaX = static_cast<float>(tileSize) / -dx;
  } else {
    stepX = 0;
    tMaxX = inf;
    tDeltaX = inf;
  }

  int stepY;
  float tMaxY, tDeltaY;
  if (dy > eps) {
    stepY = 1;
    tMaxY = (static_cast<float>(ty + 1) - fy) * static_cast<float>(tileSize) / dy;
    tDeltaY = static_cast<float>(tileSize) / dy;
  } else if (dy < -eps) {
    stepY = -1;
    tMaxY = (fy - static_cast<float>(ty)) * static_cast<float>(tileSize) / -dy;
    tDeltaY = static_cast<float>(tileSize) / -dy;
  } else {
    stepY = 0;
    tMaxY = inf;
    tDeltaY = inf;
  }

  float t = 0.0f;
  bool hitWall = false;
  while (true) {
    if (tMaxX < tMaxY) {
      t = tMaxX;
      tx += stepX;
      tMaxX += tDeltaX;
    } else {
      t = tMaxY;
      ty += stepY;
      tMaxY += tDeltaY;
    }
    if (t >= maxPx) {
      t = maxPx;
      break;
    }
    const Pos p{tx, ty};
    if (maze.outOfRange(p)) {
      break;
    }
    const Tile tile = maze[p];
    if (tile == Tile::wall || tile == Tile::door) {
      hitWall = true;
      break;
    }
  }

  outX = ax + dx * t;
  outY = ay + dy * t;
  return hitWall;
}

// Appends a 360-deg triangle-fan disc to `verts` / `indices`. Each perimeter
// vertex is the endpoint of a ray walk against the maze, capped at `radiusPx`.
// Wall-clipped so the halo never leaks through adjacent walls.
void appendHalo(
  const float ax, const float ay,
  const float radiusPx,
  const int rays,
  const MazeState &maze,
  const SDL_Color color,
  std::vector<SDL_Vertex> &verts,
  std::vector<int> &indices
) {
  const int apexIdx = static_cast<int>(verts.size());
  SDL_Vertex apexV{};
  apexV.position.x = ax;
  apexV.position.y = ay;
  apexV.color = color;
  verts.push_back(apexV);

  const int rimStart = apexIdx + 1;
  const float ext = static_cast<float>(flashlightWallEdgePx);
  for (int i = 0; i < rays; ++i) {
    const float a = 2.0f * pi_f * static_cast<float>(i) / static_cast<float>(rays);
    const float dx = std::cos(a);
    const float dy = std::sin(a);
    float ex, ey;
    const bool hit = walkRay(ax, ay, dx, dy, radiusPx, maze, ex, ey);
    if (hit) {
      ex += dx * ext;
      ey += dy * ext;
    }
    SDL_Vertex v{};
    v.position.x = ex;
    v.position.y = ey;
    v.color = color;
    verts.push_back(v);
  }

  for (int i = 0; i < rays; ++i) {
    indices.push_back(apexIdx);
    indices.push_back(rimStart + i);
    indices.push_back(rimStart + ((i + 1) % rays));
  }
}

// Appends a triangle-fan cone to `verts` / `indices`. Apex first, then
// `rays + 1` perimeter vertices distributed uniformly across the arc
// [facing - halfAngle, facing + halfAngle]. Each perimeter vertex is the
// endpoint of a ray walk against the maze.
void appendCone(
  const float ax, const float ay,
  const float facing,
  const float halfAngle,
  const float lengthPx,
  const int rays,
  const MazeState &maze,
  const SDL_Color color,
  std::vector<SDL_Vertex> &verts,
  std::vector<int> &indices
) {
  const int apexIdx = static_cast<int>(verts.size());
  SDL_Vertex apexV{};
  apexV.position.x = ax;
  apexV.position.y = ay;
  apexV.color = color;
  verts.push_back(apexV);

  const int rimStart = apexIdx + 1;
  const float ext = static_cast<float>(flashlightWallEdgePx);
  for (int i = 0; i <= rays; ++i) {
    const float t = static_cast<float>(i) / static_cast<float>(rays);
    const float a = facing + halfAngle * (2.0f * t - 1.0f);
    const float dx = std::cos(a);
    const float dy = std::sin(a);
    float ex, ey;
    const bool hit = walkRay(ax, ay, dx, dy, lengthPx, maze, ex, ey);
    if (hit) {
      ex += dx * ext;
      ey += dy * ext;
    }
    SDL_Vertex v{};
    v.position.x = ex;
    v.position.y = ey;
    v.color = color;
    verts.push_back(v);
  }

  for (int i = 0; i < rays; ++i) {
    indices.push_back(apexIdx);
    indices.push_back(rimStart + i);
    indices.push_back(rimStart + i + 1);
  }
}

}

void flashlightRender(
  entt::registry &reg,
  SDL_Renderer *renderer,
  SDL_Texture *overlay,
  const MazeState &maze,
  const int frame
) {
  const auto view = reg.view<Flashlight, Position, ActualDir, DesiredDir>();

  // Render the overlay even when no flashlight entity exists, so the maze
  // does not flash bright for a frame on edge cases. With no cones drawn the
  // playfield ends up fully black.
  SDL_Texture *const prevTarget = SDL_GetRenderTarget(renderer);
  SDL_CHECK(SDL_SetRenderTarget(renderer, overlay));
  SDL_CHECK(SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE));
  SDL_CHECK(SDL_SetRenderDrawColor(
    renderer,
    flashlightDarknessR, flashlightDarknessG, flashlightDarknessB,
    flashlightDarknessAlpha
  ));
  SDL_CHECK(SDL_RenderClear(renderer));

  std::vector<SDL_Vertex> verts;
  std::vector<int> indices;
  verts.reserve(2 * (flashlightForwardRays + flashlightBackRays + 4));
  indices.reserve(3 * (flashlightForwardRays + flashlightBackRays));

  const SDL_Color clear{0, 0, 0, 0};

  for (const entt::entity e : view) {
    const Pos posTile = view.get<Position>(e).p;
    const Dir actualDir = view.get<ActualDir>(e).d;
    const Dir desiredDir = view.get<DesiredDir>(e).d;

    const Pos subOffset = toPos(actualDir, frame);
    const float ax = static_cast<float>(posTile.x * tileSize + tileSize / 2 + subOffset.x);
    const float ay = static_cast<float>(posTile.y * tileSize + tileSize / 2 + subOffset.y);

    const float facing = facingRad(desiredDir);
    const float halfFwd = static_cast<float>(flashlightForwardHalfDeg) * pi_f / 180.0f;
    const float halfBack = static_cast<float>(flashlightBackHalfDeg) * pi_f / 180.0f;
    const float lenFwd = static_cast<float>(flashlightForwardTiles * tileSize);
    const float lenBack = static_cast<float>(flashlightBackTiles * tileSize);

    appendCone(ax, ay, facing,         halfFwd,  lenFwd,  flashlightForwardRays, maze, clear, verts, indices);
    appendCone(ax, ay, facing + pi_f,  halfBack, lenBack, flashlightBackRays,    maze, clear, verts, indices);
    appendHalo(ax, ay, static_cast<float>(flashlightHaloRadiusPx), flashlightHaloRays, maze, clear, verts, indices);
  }

  if (!indices.empty()) {
    SDL_CHECK(SDL_RenderGeometry(
      renderer,
      nullptr,
      verts.data(), static_cast<int>(verts.size()),
      indices.data(), static_cast<int>(indices.size())
    ));
  }

  SDL_CHECK(SDL_SetRenderTarget(renderer, prevTarget));

  const SDL_Rect dst{0, 0, tilesPx.x, tilesPx.y};
  SDL_CHECK(SDL_RenderCopy(renderer, overlay, nullptr, &dst));

  // Second pass: re-render the same cone geometry directly on the renderer
  // with additive blending so the lit area reads as warm yellow light. Reuses
  // the verts/indices already built; only the vertex colors are swapped.
  if (!indices.empty()) {
    const SDL_Color beam{
      flashlightBeamR, flashlightBeamG, flashlightBeamB, 255
    };
    for (SDL_Vertex &v : verts) {
      v.color = beam;
    }
    SDL_CHECK(SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD));
    SDL_CHECK(SDL_RenderGeometry(
      renderer,
      nullptr,
      verts.data(), static_cast<int>(verts.size()),
      indices.data(), static_cast<int>(indices.size())
    ));
    SDL_CHECK(SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND));
  }
}
