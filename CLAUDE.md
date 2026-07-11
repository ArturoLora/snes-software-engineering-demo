# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Native SNES reimplementation of [Apotris](https://akouzoukos.itch.io/apotris) (a guideline-compliant Tetris-like game originally written for GBA in C++/devkitARM/libtonc). This is **not** a mechanical port — it's a from-scratch C/PVSnesLib implementation that selectively reuses, adapts, or reimplements algorithms and data tables from the Apotris source where technically viable.

**Current state:** pre-implementation. Planning is complete (see `_bmad-output/`); `snes/` is an empty directory awaiting Story 1.1 (boot). There is no build yet, no toolchain installed/verified locally, and no code to lint or test.

All planning docs are written in Spanish; expect Spanish in `_bmad-output/*.md`.

## Repository layout

- `snes/` — target SNES project root (C, PVSnesLib). All new game code goes here. Empty right now.
- `reference/apotris/` — full original GBA Apotris source (C++/devkitARM/libtonc), kept locally as a reference only. **Gitignored, not tracked** — do not assume it's present in a fresh clone. Read it for logic to adapt, never build/run it as part of this project, and never copy its C++ constructs verbatim.
- `_bmad-output/` — planning artifacts produced via BMAD workflows; the source of truth for rules, architecture, and scope (see "Required Reading" below).
- `_bmad/` — BMAD tool config, not project source.

## Required Reading

Before implementing any Story, read these in order — they are the source of truth, not this file:

1. `_bmad-output/project-context.md` — binding rules/anti-patterns for this project.
2. `_bmad-output/game-architecture.md` — target C module design, ownership, frame-loop order, VRAM strategy.
3. `_bmad-output/planning-artifacts/epics.md` — full FR/NFR list and epic/story breakdown with acceptance criteria.
4. The corresponding Story file in `_bmad-output/implementation-artifacts/` — the detailed, ready-for-dev spec for the current Story.

Do not modify `project-context.md` or any other BMAD artifact — those are maintained through BMAD workflows, not directly.

## Development Philosophy

- Implement only the current Story. Do not get ahead of future Stories.
- Prefer the smallest implementation that satisfies the Story's acceptance criteria.
- Avoid speculative abstractions.
- Before writing a new algorithm, check `reference/apotris/` for an equivalent implementation first.
- Prefer adapting a proven algorithm over reinventing one, when it reduces effort and preserves the desired behavior.
- Never do a mechanical C++ → C translation — adapt the logic/data, not the syntax (classes/`std::list`/`std::tuple` have no equivalent in C).

## Git workflow

- Claude must never execute Git commands that modify repository history.
- Never run `git add`, `git commit`, `git push`, `git pull --rebase`, `git merge`, `git rebase`, `git tag`, or create/delete branches.
- Claude may suggest Git commands when useful, but Arturo is the only person who performs Git operations.
- Never finish a workflow by committing or pushing automatically.

## Token Budget

Para este proyecto:

- Preferir workflows pequeños e incrementales.
- Evitar subagentes salvo que el usuario los solicite.
- Evitar investigaciones web cuando el problema pueda resolverse inspeccionando el repositorio o el entorno local.
- Cada Story debe poder implementarse en una sola sesión.
- Antes de usar workflows de revisión o investigación costosos, proponer una alternativa de bajo consumo de tokens.

## Validation

Cada Story debe terminar en este orden:

1. make
2. Ejecutar en ares
3. Esperar validación manual del desarrollador
4. Solo entonces considerar la Story terminada

Nunca asumir que una Story está completa únicamente porque compila.