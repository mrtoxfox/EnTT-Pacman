# Task
Implement Fog of War in this C++17 / SDL2 / EnTT Pac-Man clone. Unvisited maze areas are hidden. Tiles reveal permanently as Pac-Man moves near them. Ghosts under fog are invisible.

## Step 1 - Branch
Generate 3 fundamentally different strategies for rendering and tracking fog. Each must be a complete, independent approach, not a variation of another.

## Step 2 - Evaluate
Score each strategy 1-5 on:
- Performance within the existing game loop
- Visual quality
- Code complexity
- Integration risk, and fit with the ECS / EnTT conventions in `CLAUDE.md`

## Step 3 - Prune
Eliminate the weakest strategy. Explain why it loses.

## Step 4 - Deepen
Take the 2 survivors one level deeper. Cover at minimum:
- What is the reveal radius - how many tiles around Pac-Man get permanently uncovered?
- Does fog reset between lives, or persist across deaths?
- Does fog reset on level restart?
- How does fog interact with the tunnel wrap-around at row y=10?
- The ghost house - does it start revealed, or stay fogged until Pac-Man approaches?
- Dots and energizers under fog - fully hidden, or faintly hinted to keep the game playable?
- Ghosts crossing between fogged and revealed areas - exactly when they appear/disappear
- Does the fog edge look clean, or jagged/torn along walls?
- How the fog overlay interacts with the score display and win/lose state
- Cost of tracking revealed state for every cell, and where that state lives (grid alongside the maze, or in the ECS)

## Step 5 - Select
Choose the winner. Justify with a direct comparison.

## Step 6 - Plan
Produce a step-by-step implementation plan from the winning strategy.