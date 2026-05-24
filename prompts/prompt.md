# Task
Design a universal bonus system for this EnTT-based Pac-Man clone, then plan its implementation. Do not write code yet.

## Features
- A bonus system that spawns bonuses randomly on valid dot cells of the maze, never on walls, centered in the tile grid.
- Freeze bonus - freezes all ghosts for 10 seconds.
- Slow bonus - halves ghost speed for 10 seconds.
- Speed bonus - doubles ghost speed for 10 seconds.

## Constraints
- The system must follow the project's ECS / EnTT conventions: bonuses are components plus their own systems, decoupled from existing ghost and player logic.
- A sound plays on bonus spawn, on effect applied, and on effect expired.

## Approach
Let's think step by step. Before any code, explain your reasoning for each step:
1. Examine - which existing components and systems are involved, and what state they expose.
2. Design - how the bonus system is structured: spawning, collection, effect, expiry.
3. Integrate - the minimal changes and where they fit in the `Game::logic()` system order.
4. Order - what must be built first.