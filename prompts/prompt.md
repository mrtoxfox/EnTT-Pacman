Act as a Senior Software Architect. Analyze the current project and write technical documentation in Markdown, saved to `docs/architecture.md`.

Include these sections: Project Overview, System Architecture, Data Flow, and a table of contents added as the last step.

You must include a Mermaid `flowchart` for the ECS architecture (registry, components, systems) and a Mermaid `sequenceDiagram` for one logic step of the game loop, both in ```mermaid blocks.

Context: a Pac-Man clone in C++17 using SDL2 for input/rendering and the EnTT framework for an Entity-Component-System design, with a two-rate game loop (logic vs render). See `CLAUDE.md` for details.