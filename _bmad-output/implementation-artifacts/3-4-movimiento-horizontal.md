---
baseline_commit: fcb25f65a5adf6478628bee305a39ffedd3e35d1
---

# Story 3.4: Movimiento horizontal de la pieza activa

Status: in-progress (implementación + build verificados; confirmación visual en emulador pendiente del usuario)

## Story

Como desarrollador,
quiero `piece_move_left()`/`piece_move_right()` que muevan `piece.x` solo si la celda destino no está ocupada,
para tener el primer movimiento validado por colisión, antes de leer input real o agregar DAS/ARR.

## Alcance (fijado explícitamente por el usuario)

**Sí implementar:**
- Ampliar `snes/source/piece.c`/`piece.h` con `void piece_move_left(GameState *gs)` y `void piece_move_right(GameState *gs)`.
- Mover únicamente `gs->piece.x` (±1) — nada de `y`, `rotation` ni `type`.
- Usar `board_is_cell_occupied()` (Story 2.2) para impedir salir del tablero — si la celda destino está ocupada/fuera de rango, no mover.
- La pieza sigue fija (`type=0`, `rotation=0` — sin cambios de `piece_spawn()`, Story 3.3).
- Prueba por consola: mostrar `piece.x` antes y después de llamar `piece_move_left()`/`piece_move_right()`.

**NO implementar todavía:**
- Lectura de pad real (`input.c` — Epic 4/Story 4.2; esta story llama las funciones directamente desde `main.c`, sin pad).
- DAS/ARR (auto-repeat).
- Gravedad, lock, line clear.
- Render (sprites/OAM).
- Rotación, SRS, kicks.
- Colisión de la **forma completa** de la pieza (las 4 celdas de `piece_shapes[type][rotation]`) — Apotris valida la forma 4×4 completa en `checkRotation()`; nosotros todavía no tenemos esa función (solo `board_is_cell_occupied()`, que resuelve una única celda, Story 2.2). Esta story valida el movimiento contra `piece.x`/`piece.y` como punto único, una simplificación explícita — ver Dev Notes.

## Acceptance Criteria

1. **Given** `gs->piece.x` dentro de rango (ej. tras `piece_spawn()`, `x=3`) y la celda `(x-1, y)` libre, **when** se llama `piece_move_left(gs)`, **then** `gs->piece.x` decrece en 1.
2. **Given** `gs->piece.x = 0` (borde izquierdo), **when** se llama `piece_move_left(gs)`, **then** `gs->piece.x` no cambia (bloqueado por `board_is_cell_occupied` al detectar `x-1 < 0`).
3. **Given** `gs->piece.x` dentro de rango y la celda `(x+1, y)` libre, **when** se llama `piece_move_right(gs)`, **then** `gs->piece.x` incrementa en 1.
4. **Given** `gs->piece.x = BOARD_WIDTH-1` (borde derecho), **when** se llama `piece_move_right(gs)`, **then** `gs->piece.x` no cambia.
5. **Given** el `main.c` existente, **when** arranca la ROM, **then** se imprime por consola `piece.x` antes y después de llamar ambas funciones de movimiento, confirmando visualmente el comportamiento de los AC #1-#4.
6. **Given** el proyecto compila con `make`, **when** se agregan las 2 funciones nuevas, **then** no hay errores de compilación/link y las Stories anteriores (1.1 a 3.3) siguen funcionando sin cambios.

## Tasks / Subtasks

- [x] **Task 1: `piece_move_left`/`piece_move_right` en `piece.h`/`piece.c`** (AC: #1, #2, #3, #4)
  - [x] Declaradas en `piece.h` junto a `piece_spawn`.
  - [x] `piece_move_left`: `s8 new_x = gs->piece.x - 1; if (!board_is_cell_occupied(gs, new_x, gs->piece.y)) gs->piece.x = new_x;`.
  - [x] `piece_move_right`: simétrico con `+1`.
  - [x] `piece.c` ahora incluye `board.h` además de `game_state.h`.
  - [x] Sin `moveHistory`/`pushDir`/`das`/`lockCheck()`/`sounds.shift`/`gameMode`.

- [x] **Task 2: Prueba mínima en `main.c`** (AC: #5, #6)
  - [x] Fila 24: `x_before_left` capturado en variable local antes de `piece_move_left(&gs)`, impreso "MOVE LEFT X: %d -> %d" (antes → después).
  - [x] Fila 26: mismo patrón con `piece_move_right(&gs)`, "MOVE RIGHT X: %d -> %d".
  - [x] Todos los valores `s8`/`u8` casteados a `s16`/`u16` antes de `consoleDrawText` (bug de la Story 3.3).
  - [x] Loop principal, pad, BG0/BG1, `board.c`, `piece_data.c`, `piece_spawn()` y pruebas de Stories anteriores (filas 12, 16, 20, 22) sin cambios.

## Dev Notes

- **Origen de la lógica (Apotris, solo lectura, líneas citadas):** `reference/apotris/source/tetrisEngine.cpp:286-332` (`Game::moveLeft()`/`Game::moveRight()`). No se leyó ninguna otra función de ese archivo para esta story (ya se leyeron `checkRotation` en la Story 2.2 y `Game::next()` en la 3.3, no se releyeron).
  - Núcleo: `if (checkRotation(∓1, 0, pawn.rotation)) { pawn.x∓∓; ...; return true; } else { ...; return false; }`.
  - `moveHistory` (líneas 287-294, 311-318) es de finesse (fuera de alcance de todo el proyecto). `lockCheck()` (líneas 300, 324) resetea el lock-timer (Story futura de lock). `sounds.shift`/`gameMode == CLASSIC` (líneas 298, 304, 322, 328) son audio/modos (fuera de alcance).
- **Simplificación deliberada de la colisión:** Apotris valida la forma 4×4 completa de la pieza (`checkRotation` itera `pawn.board[r][i][j]`, las 4 celdas ocupadas de esa rotación). Nuestro proyecto todavía no tiene esa función — solo `board_is_cell_occupied(gs, x, y)` de la Story 2.2, que resuelve una única celda. Esta story usa `piece.x`/`piece.y` (la posición ancla de la pieza) como si fuera un punto único contra el tablero, tal como pidió el usuario ("utilizar `board_is_cell_occupied()` para impedir salir del tablero"). Cuando exista colisión de forma completa (story futura, usará `piece_shapes[type][rotation]`), este chequeo de un solo punto se reemplaza — no hace falta migrar código, solo agregar la nueva función y usarla en su lugar.
- Sin cambios a `piece_spawn()` (Story 3.3), `board_is_cell_occupied`/`board_get`/`board_set` (Stories 2.1/2.2), ni `piece_shapes` (Story 3.2).
- Memoria estática, sin alloc dinámica, C puro (`project-context.md`). `piece.c` sigue siendo el único módulo que escribe `gs->piece`.

### References

- `reference/apotris/source/tetrisEngine.cpp:286-332` (`Game::moveLeft()`/`Game::moveRight()`, únicas funciones leídas de ese archivo en esta story)
- `_bmad-output/game-architecture.md#2 Módulos y responsabilidad` (`piece.c`: "movimiento horizontal (DAS/ARR)... invoca `board_*` para colisión" — DAS/ARR explícitamente fuera de esta story, per pedido del usuario)
- `_bmad-output/project-context.md#Alcance mínimo, Anti-patrones para agentes`
- `_bmad-output/implementation-artifacts/2-2-colision-basica.md` (`board_is_cell_occupied`, ya existente)
- `_bmad-output/implementation-artifacts/3-3-spawn-inicial.md` (`piece_spawn()`, `piece.c`/`piece.h` ya existentes; también documenta el bug de `consoleDrawText` con tipos `u8`/`s8` — aplicar el mismo cast en la prueba de esta story)

## Dev Agent Record

### Agent Model Used

claude-sonnet-5

### Debug Log References

- `make` en `snes/` (PVSNESLIB_HOME=~/Tools/pvsneslib): compiló y linkeó sin errores con `piece_move_left`/`piece_move_right` sumadas a `piece.c`. `make clean` corrido al final.

### Completion Notes List

- Colisión puntual (no de forma completa) mantenida tal como pide la Story — `board_is_cell_occupied(gs, new_x, gs->piece.y)` valida un solo punto, no las 4 celdas de `piece_shapes[type][rotation]`.
- Prueba en `main.c` captura `x` en una variable local (`s16 x_before_left`/`x_before_right`) antes de mover, en vez de reconstruir el valor "antes" con aritmética sobre el estado post-movimiento — evita mostrar un "antes" incorrecto si el movimiento no ocurre (choque contra el borde).
- Todos los valores nuevos pasados a `consoleDrawText` casteados a `s16`/`u16` (bug de varargs corregido en la Story 3.3).
- No se tocó `piece_spawn()`, `board.c`/`board.h`, `piece_data.c`/`.h`, `game_state.h` ni ninguna Story previa.
- **No verificado por el agente:** resultado visual en emulador (AC #5) — no hay emulador en este entorno; pendiente de confirmación del usuario, igual que en las Stories anteriores.

### File List

- `snes/source/piece.h` (modificado) — declaraciones de `piece_move_left`/`piece_move_right`.
- `snes/source/piece.c` (modificado) — implementación de ambas; `#include "board.h"` agregado. `piece_spawn()` sin cambios.
- `snes/source/main.c` (modificado) — prueba mínima en filas 24/26. Resto sin cambios.

### Change Log

| Fecha | Cambio |
| --- | --- |
| 2026-07-11 | Implementación de Story 3.4: `piece_move_left`/`piece_move_right` en `piece.c`, colisión puntual vía `board_is_cell_occupied`, adaptado de `Game::moveLeft()`/`moveRight()` de Apotris. `make` compila sin errores. Status → in-progress (confirmación visual en emulador pendiente del usuario). |
