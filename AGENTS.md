# Repository Guidelines

- このリポジトリに関するエージェントからの回答は日本語で行ってください。

## Project Structure & Module Organization
- `main.cpp`: Core game loop, stage parsing, rendering, and input handling. Keep gameplay logic colocated and prefer small helper functions over new files unless a subsystem grows large.
- `img/`: Sprite and texture assets referenced via `Assets::` constants; replace files without renaming to avoid code changes.
- `1-1.map`: Tile map consumed by `Stage::load_stage`; use the same symbols when adding new maps.
- `build/`: Generated binaries and CMake artifacts. Avoid committing its contents.

## Build, Test, and Development Commands
- First-time configure: `cmake -S . -B build` (generates Ninja/Make files with C++17 and SDL2/SDL2_image discovery).
- Build: `cmake --build build` (produces `build/mario` with warnings enabled on Clang/GCC).
- Run locally: `./build/mario` (launches SDL window; ensure `img/` is present in the working directory).

## Coding Style & Naming Conventions
- Language: C++17, prefer standard library containers and algorithms.
- Indentation: 4 spaces; keep brace style consistent with existing code.
- Naming: Types/structs/classes use PascalCase, functions/methods camelCase, constants in `Assets::` remain SCREAMING_SNAKE_CASE; keep tile/pipe/item enums in sync with map symbols.
- Dependencies: Keep texture paths centralized in `Assets` and stage tables initialized in `Stage::initTileTable` to avoid scattering literals.

## Testing Guidelines
- Automated tests are not yet present. Validate changes by running `./build/mario` and checking common flows: movement/jump, item box drops, enemy collisions, warp pipes, and stage transitions (including underground markers `*` in map files).
- When adding tests later, mirror current naming (e.g., `StageLoadTests`) and place them under `tests/` with CTest integration.

## Commit & Pull Request Guidelines
- Commits: Use short, imperative subjects mirroring history (e.g., `add ocean-mode`, `fix fire-action`). Group related gameplay or asset tweaks together.
- Pull Requests: Include a summary of gameplay changes, affected assets/maps, reproduction steps, and before/after screenshots or short clips when visuals change. Link related issues and call out new dependencies (SDL versions, new assets) explicitly.

## Security & Configuration Tips
- Assets load by relative path; run from repo root or configure your IDE to set the working directory accordingly.
- SDL2/SDL2_image are required; install via Homebrew (`brew install sdl2 sdl2_image`) or your platform’s equivalent before configuring CMake.
