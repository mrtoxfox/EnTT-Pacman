# Task
Analyze the current game loop, endgame mechanics, and scoring. Then create a detailed plan describing how the following features should work and how to implement them in this project, and save it to `prompts/plan.md` in markdown format.

The plan must follow the project's ECS / EnTT architecture and conventions (see `CLAUDE.md`). Do not write any code yet.

## Features
1. **Implement score system.** Pac-Man earns points for eating dots, energizers, and scared ghosts. Use standard arcade values (dots: 10, energizers: 50, ghosts: 200). Score lives on the player entity as a component.
2. **Implement pause mechanic** with a pause screen. While paused, all game logic freezes and audio stops. The pause screen should follow the end-game screen style (translucent overlay over the frozen scene, not a new sprite). Key bindings are defined in item 5.
3. **Implement lives for Pac-Man.** Pac-Man starts with 3 lives. When caught by a ghost, he loses 1 life and half his score (integer division). The game only ends when all 3 lives are gone.
4. **Display scores and lives on screen.** Show the current score and remaining lives count as part of the in-game HUD. On the lose screen, also display the final score and lives spent (i.e. how many lives were lost before game over).
5. **Pause screen text and remapped keys.** Show a centered "PAUSED" text on the pause screen, in the lose-screen visual style. Remap the keys: `Esc` exits the game; `Space` toggles the visual pause (overlay + text + audio paused); `P` toggles a debug pause that only freezes the game (no overlay, no text) so a single frame can be inspected. The debug pause must freeze at the exact moment the key is pressed (no extra frame, no sub-tile jump).
6. **Winner screen parity with lose screen.** The win screen should also display the final score and spent-lives row, in the same end-screen style as the lose screen.

Stop after writing `prompts/plan.md`. I'll review it, edit it, then ask you to implement.
