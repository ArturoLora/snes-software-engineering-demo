---
baseline_commit: 4d637ff46d93c9b088fc0d1deb27b3e2385ac1cb
---

# Story 4.2: Input real del pad

Status: in-progress (implementación + build verificados; confirmación visual en emulador pendiente del usuario)

## Story

Como jugador,
quiero controlar la pieza activa con el pad real (SNES o emulador),
para jugar sin depender de ningún input simulado de depuración.

## Alcance (mismo tamaño que las Stories anteriores)

**Sí implementar:**
- Nuevo módulo `input.c`/`input.h` — única frontera con `pad_keys`/`pad_keysdown` de PVSnesLib (`game-architecture.md#2, #6`). Ningún otro módulo de gameplay (`board.c`, `piece.c`, `piece_data.c`, `queue.c`) incluye ni referencia constantes de PVSnesLib.
- `InputIntent input_read(void)`: struct `{u8 left; u8 right; u8 down;}` (no vive en `GameState`, `game-architecture.md#4`), producida leyendo `padsDown(0)` (`pad_keysdown[0]`, edge-triggered — un paso por pulsación, sin DAS/ARR, igual que Story 3.3/3.4).
- Conectar `input_read()` al loop principal (`while(1)`) de `main.c`: si `intent.left` → `piece_move_left(&gs)`; si `intent.right` → `piece_move_right(&gs)`; luego `render_sync_piece(&gs, 1)` para reflejar la nueva posición cada frame.
- Un `piece_spawn(&gs)` nuevo justo antes de entrar al `while(1)`, para que el loop interactivo empiece con una pieza activa fresca (no la que ya quedó fijada/lockeada por la secuencia de arranque de las Stories anteriores).

**NO implementar todavía:**
- Gravedad ni lock dentro del loop en vivo (la Story no lo pide; sigue siendo Q8.8/tiempo real fuera de alcance).
- `intent.down` no dispara nada todavía (el campo existe en `InputIntent` porque `game-architecture.md#4` ya lo define, pero esta Story solo conecta izquierda/derecha).
- DMA del tilemap — Story 4.3.
- Rotación, SRS, 7-bag/queue real.
- Tocar el `switch` de depuración ya existente en el `while(1)` (impresión del nombre de tecla presionada, Story 1.1) — no es lógica de gameplay, queda igual.
- Cambios a `board.c`, `piece.c`, `piece_data.c`, `game_state.h`, `render.c` — ninguno necesita cambios (solo se los invoca).

## Acceptance Criteria

1. **Given** `input.c` traduciendo `pad_keys`/`pad_keysdown` (`KEY_LEFT`/`KEY_RIGHT`), **when** el jugador presiona izquierda o derecha en el pad, **then** la pieza en pantalla se mueve según la Story 3.3 (un paso por pulsación, bloqueado si colisiona), usando el input real.
2. **Given** el resto de los módulos de gameplay (`board.c`, `piece.c`, `piece_data.c`, `queue.c` — aún no existe), **when** se revisan, **then** ninguno referencia directamente constantes de PVSnesLib (`pad_keys`, `KEY_LEFT`, etc.) — solo `input.c`.
3. **Given** el proyecto compila con `make`, **when** se agrega `input.c/.h` y su integración en `main.c`, **then** no hay errores de compilación/link y las Stories anteriores (1.1-4.1) siguen funcionando.
4. **Given** la ROM corre en Ares, **when** el jugador presiona izquierda/derecha, **then** el sprite de la pieza (Story 4.1) se mueve visualmente en la dirección correspondiente — confirmado manualmente por el usuario (no verificable por el agente).

## Tasks / Subtasks

- [x] **Task 1: `input.h`/`input.c` nuevos** (AC: #1, #2)
  - [x] `InputIntent { u8 left; u8 right; u8 down; }`.
  - [x] `InputIntent input_read(void)`: lee `padsDown(0)` una sola vez, arma los 3 flags con `KEY_LEFT`/`KEY_RIGHT`/`KEY_DOWN`.
  - [x] `input.c` no incluye `game_state.h` (no conoce `GameState`, `game-architecture.md#6`).

- [x] **Task 2: Integrar en `main.c`** (AC: #1, #3, #4)
  - [x] `#include "input.h"`.
  - [x] `piece_spawn(&gs)` justo antes del `while(1)`.
  - [x] Dentro del `while(1)`: `InputIntent intent = input_read(); if (intent.left) piece_move_left(&gs); if (intent.right) piece_move_right(&gs); render_sync_piece(&gs, 1);`.
  - [x] No se tocó el `switch`/impresión de tecla de la Story 1.1.

- [x] **Task 3: Build** (AC: #3, #4)
  - [x] `make` compila y linkea sin errores; Stories 1.1-4.1 siguen funcionando.
  - [x] `make clean` **no** ejecutado — ROM disponible para Ares.

## Dev Notes

- **Ownership (`game-architecture.md#2, #6`):** `input.c` es la única frontera con `pad_keys[5]`/`pad_keysdown[5]` de PVSnesLib; produce `InputIntent` aparte de `GameState` — ningún otro módulo de gameplay depende del nombre exacto de las constantes de PVSnesLib.
- **`padsDown` vs `padsCurrent`:** `padsCurrent(id)` (`pad_keys[id]`) ya usado en `main.c` para el debug print de tecla — es *held* (nivel), no edge. Para movimiento un-paso-por-pulsación (Story 3.3/3.4, sin DAS/ARR) corresponde `padsDown(id)` (`pad_keysdown[id]`, confirmado en `include/snes/input.h:93,206` — newly-pressed-down keys), no `padsCurrent`.
- **Por qué un `piece_spawn()` nuevo antes del loop:** la secuencia de arranque (Stories 3.3-3.7/4.1) ya deja la pieza de prueba fijada (`piece_lock()`) antes de llegar al `while(1)` — sin un spawn nuevo, el loop movería una pieza ya lockeada, sin sentido de juego. Este spawn no toca ninguna de las pruebas existentes (todas terminan antes de `bgSetEnable(1)`/`setScreenOn()`), solo agrega el punto de partida para el loop interactivo.
- **Sin gravedad/lock en el loop:** no lo pide esta Story; `piece_apply_gravity()`/`piece_lock()` siguen sin conectarse a un ciclo en tiempo real — eso implicaría diseñar un acumulador de tiempo/frames que ninguna Story actual cubre.
- No se leyó ninguna función nueva de Apotris para esta Story — la lectura de pad es 100% específica de PVSnesLib (`pad_keysdown`), sin equivalente de Apotris a adaptar (Apotris lee input de hardware GBA, libtonc).

### Project Context Rules

- Memoria estática, sin allocación dinámica (`project-context.md#Defaults de implementación`).
- C puro, sin C++/STL (`project-context.md#Stack técnico objetivo`).
- No implementar features fuera del alcance mínimo fijado arriba sin pedido explícito (`project-context.md#Anti-patrones para agentes`).
- Validación: al finalizar, ejecutar únicamente `make` (no `make clean`) — la ROM debe quedar disponible para Ares. Secuencia completa: 1) `make`, 2) ejecutar en Ares, 3) esperar validación manual del usuario, 4) recién entonces considerar la Story terminada.

### References

- `_bmad-output/planning-artifacts/epics.md#Epic 4 / Story 4.2`.
- `_bmad-output/game-architecture.md#2 Módulos y responsabilidad, #4 Estructuras de datos principales (InputIntent), #6 Input PVSnesLib`.
- `_bmad-output/project-context.md#Defaults de implementación, Anti-patrones para agentes`.
- `_bmad-output/implementation-artifacts/4-1-render-pieza-sprites.md` (`render_sync_piece`, ya integrado en el loop de pruebas; Story previa inmediata).
- `_bmad-output/implementation-artifacts/3-4-movimiento-horizontal.md` (`piece_move_left`/`piece_move_right`, contrato ya establecido).
- `/home/arturo/Tools/pvsneslib/pvsneslib/include/snes/input.h:91-93,198,206` (`pad_keys`/`pad_keysdown`/`padsCurrent`/`padsDown`, confirmado localmente).

## Dev Agent Record

### Agent Model Used

claude-sonnet-5

### Debug Log References

- `make`: compiló y linkeó sin errores (ROM 90.58% libre). `make clean` no ejecutado.

### Completion Notes List

- `input.c`/`input.h` nuevos: `input_read(void)` lee `padsDown(0)` (`pad_keysdown`, edge-triggered) y arma `InputIntent{left,right,down}`. Único módulo que referencia `KEY_LEFT`/`KEY_RIGHT`/`KEY_DOWN` fuera del `switch` de debug pre-existente en `main.c` (Story 1.1, sin tocar).
- `main.c`: agregado `piece_spawn(&gs)` + `render_sync_piece(&gs,1)` justo antes de `bgSetEnable(1); setScreenOn();` (reemplaza el llamado de "diagnostic fix" que había quedado de la Story 4.1 — misma posición, ahora con propósito real: dar al loop una pieza fresca, no la ya lockeada por las pruebas de boot). Dentro del `while(1)`: `input_read()` + `piece_move_left`/`piece_move_right` según `intent.left`/`intent.right` + `render_sync_piece(&gs,1)` cada frame.
- Sin gravedad ni lock conectados al loop en vivo — fuera de alcance de esta Story. `intent.down` no dispara nada todavía.
- No se tocó `board.c`, `piece.c`, `piece_data.c`, `game_state.h`, `render.c`, ni el `switch` de depuración de teclas.
- **No verificado por el agente:** movimiento visual real en Ares (AC #4) — no hay emulador en este entorno; pendiente de confirmación manual del usuario.

### File List

- `snes/source/input.h` (nuevo) — `InputIntent`, declaración de `input_read`.
- `snes/source/input.c` (nuevo) — implementación de `input_read` (única frontera con pad_keys/pad_keysdown).
- `snes/source/main.c` (modificado) — `#include "input.h"`; `piece_spawn`+`render_sync_piece` antes del `while(1)`; lectura de input real + movimiento + render dentro del loop. Comentario de cabecera actualizado con la entrada de Story 4.2.

### Change Log

| Fecha | Cambio |
| --- | --- |
| 2026-07-11 | Implementación de Story 4.2: `input.c`/`.h` nuevos (única frontera con pads de PVSnesLib); loop principal de `main.c` ahora mueve la pieza activa con input real (`padsDown`) en vez de solo la secuencia de arranque simulada. `board.c`/`piece.c`/`game_state.h`/`render.c` sin cambios. `make` compila y linkea sin errores; `make clean` no ejecutado, ROM disponible para Ares. Status → in-progress (confirmación visual pendiente del usuario). |
