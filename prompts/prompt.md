# Task
Add a flashlight with true 2D shadow casting to this Pac-Man clone (C++17 / SDL2 / EnTT). The play field is fully black (alpha 255) outside the beam: no walls, dots, ghosts, or silhouettes visible. Pac-Man emits a cone-shaped beam in his facing direction plus a smaller, shorter back cone. The beam has hard edges, no falloff (binary lit / unlit). Walls **cast shadows**: the lit region is the geometric intersection of the directional cone and the visibility polygon from Pac-Man's position. Corridors that bend out of his line of sight stay dark even when inside the cone's angular span. Read `CLAUDE.md` first.

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
- Outside the lit region everything is pure black (no dim outlines, no faint ghosts, no dots). Inside the lit region everything renders unchanged
- Dots and energizers must not influence illumination rendering (no beam widening on energizer pickup, no per-dot light)
- Pac-Man himself is always fully visible because he sits at the cone apex; no extra halo logic unless the cone math fails to cover his sprite
- Wall edges inside the lit region render as clean continuous lines, not jagged at the cone edge or shadow boundary
- Ghost AI and collision are unchanged. Only rendering changes
- The SDL2 compositing approach: render-target texture, blend modes, alpha values, and which SDL2 API call fills the lit polygon (verify SDL2 version availability)

## Step 5 - Select
Choose the winner. Justify with a direct comparison against the runner-up. Recommend concrete values for forward cone length (tiles), back cone length (tiles), forward half-angle, back half-angle, and ray count per cone.

## Step 6 - Plan
Produce a step-by-step implementation plan from the winning strategy. The plan must respect the conventions in `CLAUDE.md`: EnTT 3.4.0 API only, plain-aggregate components, stateless free-function systems with `entt::registry &` first param, tunables as `constexpr` in `core/constants.hpp`, no new state outside the ECS except the cached render-target texture, clean compile under `-Wall -Wextra -Wpedantic`.

## Deliverable
One PR with the implementation, four screenshots (one per facing direction) each showing at least one visible wall shadow, and a PR description naming the chosen SDL2 polygon-fill call, the required SDL2 version, and the chosen tunable values.

## Out of scope
Battery / fuel. Coloured light. Soft / gradient cone edges or brightness falloff. Beam changes from energizers or ghost mode. 360-degree ambient halo. Any change to gameplay logic, AI, or collision.
