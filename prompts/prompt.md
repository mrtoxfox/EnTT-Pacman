# Task
Add a flashlight with true 2D shadow casting to this Pac-Man clone (C++17 / SDL2 / EnTT). The play field is fully black (alpha 255) outside the beam: no walls, dots, ghosts, or silhouettes visible. Pac-Man emits a cone-shaped beam in his facing direction plus a smaller, shorter back cone, with a small always-lit halo around his sprite so he is never clipped. The beam has hard angular edges (no soft falloff) and reads as a faint warm yellow tint over the normal maze art. Walls **cast shadows**: the lit region is the geometric intersection of the directional cones (plus the halo disc) and the visibility polygon from Pac-Man's position. Corridors that bend out of his line of sight stay dark even when inside the cone's angular span. Wall tiles whose inner face borders the lit region are themselves visible, so corridor walls under the beam read as solid lines rather than dark slabs. Read `CLAUDE.md` first.

## Step 1 - Branch
Generate 3 fundamentally different strategies for computing and rendering the lit region with shadow casting. Each must be a complete, independent approach (e.g. ray-cast visibility polygon, tile-level BFS visibility, shader-style stencil mask), not a variation of another.

## Step 2 - Evaluate
Score each strategy 1-5 on:
- Performance within the existing two-rate game loop
- Visual quality (crisp shadow edges, smooth cone motion at sub-tile resolution, no artifacts)
- Wall occlusion accuracy (light stops at walls, no leaks through corners, tunnel wrap does not project light)
- Code complexity
- Integration risk, and fit with the ECS / EnTT 3.4.0 conventions in `CLAUDE.md`

## Step 3 - Prune
Eliminate the weakest strategy. Explain why it loses.

## Step 4 - Deepen
Take the 2 survivors one level deeper. Cover at minimum:
- The math for cone shape and wall-blocked visibility (rays, polygon construction, corner casting)
- Behavior in narrow corridors, at corners, and at intersections, including side branches inside the cone
- Sub-tile interpolation: the cone apex follows Pac-Man's interpolated pixel position, not his tile position
- Per-frame cost given the two-rate loop (logic at fps/8, render every frame)
- Interaction with existing overlays: HUD is never darkened; the flashlight effect applies during `playing` and both pause states but is skipped on `won` / `lost`
- Pause behavior: the cone freezes in place along with the rest of the world; PAUSED text stays readable
- Tunnel wrap (row y=10): light does not wrap, it stops at the tunnel exit
- Ghost-house door: treated as an opaque wall for visibility
- Outside the lit region everything is pure black (no dim outlines, no faint ghosts, no dots). Inside the lit region the maze, dots, and ghosts render normally with a faint warm yellow tint added on top so the beam reads as light
- Dots and energizers must not influence illumination rendering (no beam widening on energizer pickup, no per-dot light)
- A small always-lit halo disc around Pac-Man guarantees his sprite is fully covered even when the cones leave a perpendicular gap. The halo is occluded by walls like the cones are
- Wall tiles bordering the lit region show their inner face under the beam, tile-aligned so they read as smooth continuous lines rather than zigzagged or broken short segments. Walls outside the cone arc stay dark even if they are adjacent to a lit corridor cell
- Ghost AI and collision are unchanged. Only rendering changes
- The SDL2 compositing approach: render-target texture, blend modes, alpha values, and which SDL2 API call fills the lit polygon (verify SDL2 version availability)

## Step 5 - Select
Choose the winner. Justify with a direct comparison against the runner-up. Recommend concrete values for forward cone length (tiles), back cone length (tiles), forward half-angle, back half-angle, ray count per cone, halo radius (pixels) and halo ray count, and the RGB of the warm beam tint.

## Step 6 - Plan
Produce a step-by-step implementation plan from the winning strategy. The plan must respect the conventions in `CLAUDE.md`: EnTT 3.4.0 API only, plain-aggregate components, stateless free-function systems with `entt::registry &` first param, tunables as `constexpr` in `core/constants.hpp`, no new state outside the ECS except the cached render-target texture, clean compile under `-Wall -Wextra -Wpedantic`.

## Deliverable
One PR with the implementation, four screenshots (one per facing direction) each showing at least one visible wall shadow and lit wall faces inside the beam, and a PR description naming the chosen SDL2 polygon-fill call, the required SDL2 version, and the chosen tunable values.

## Out of scope
Battery / fuel. Coloured directional light beyond the faint warm tint. Soft / gradient cone edges or distance-based brightness falloff. Beam changes from energizers or ghost mode. A full-screen ambient halo (the small player-centered halo is in scope; lighting the whole maze is not). Any change to gameplay logic, AI, or collision.
