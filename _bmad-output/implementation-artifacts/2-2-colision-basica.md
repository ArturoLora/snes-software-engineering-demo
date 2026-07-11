---
baseline_commit: 0a3fff208dbf31e4f7793cb56ab917ae56779feb
---

# Story 2.2: Colisión contra bordes y celdas ocupadas

Status: in-progress (implementación + build verificados; confirmación visual en emulador pendiente del usuario)

## Story

Como desarrollador,
quiero una función de colisión (`board_is_cell_occupied`) que determine si una celda está fuera de los bordes del tablero o ya ocupada,
para que cualquier movimiento futuro de pieza (Epic 3) pueda validarse contra el tablero antes de aplicarse.

## Alcance (fijado explícitamente por el usuario)

**Sí implementar:**
- `board_is_cell_occupied(GameState *gs, s8 x, s8 y)` en `board.c`/`board.h` (mismo módulo de la Story 2.1) — bordes izquierdo/derecho, piso, y celda ocupada.
- Reutiliza `GameState`/`board_get` ya existentes (Story 2.1) — no se toca `board_set`/`board_clear`/`board_init`.
- Prueba mínima en `main.c` (mismo patrón de la Story 2.1): imprimir por consola de texto el resultado en los 3 casos exigidos por `epics.md` (fuera de límite, celda ocupada, celda vacía).

**NO implementar todavía** (aunque `game-architecture.md`/Apotris las mezclen en el mismo código fuente): spawn, gravedad, lock, line clear, rotación, SRS, kicks, 7-bag, ni ninguna pieza/`ActivePiece`. Esta story es solo la función de colisión contra el tablero — nada que dependa de la forma de una pieza.

## Acceptance Criteria

1. **Given** una celda dentro de los límites del tablero (`0 <= x < 10`, `0 <= y < 22`) y vacía, **when** se llama `board_is_cell_occupied`, **then** devuelve "no ocupada".
2. **Given** una celda dentro de límites ya escrita con `board_set` (valor != 0), **when** se llama `board_is_cell_occupied` sobre esa celda, **then** devuelve "ocupada".
3. **Given** coordenadas fuera de límites (x < 0, x >= 10, y < 0, o y >= 22), **when** se llama `board_is_cell_occupied`, **then** devuelve "ocupada" (bloqueado) — igual que un borde/piso real, sin leer el array fuera de rango.
4. **Given** el `main.c` existente, **when** arranca la ROM, **then** se imprime por consola de texto el resultado de los 3 casos de arriba (límite, ocupada, vacía), confirmando visualmente el comportamiento.
5. **Given** el resto de los módulos del proyecto, **when** se revisa el código, **then** `board_is_cell_occupied` es la única función nueva de este story y no introduce ningún struct de pieza, tabla de formas, ni lógica de spawn/gravedad/lock.

## Tasks / Subtasks

- [x] **Task 1: `board_is_cell_occupied` en `board.h`/`board.c`** (AC: #1, #2, #3, #5)
  - [x] Firma implementada tal cual: `u8 board_is_cell_occupied(GameState *gs, s8 x, s8 y)`.
  - [x] `if (x < 0 || x >= BOARD_WIDTH || y < 0 || y >= BOARD_HEIGHT) return 1; return board_get(gs, (u8)x, (u8)y) != 0;` — idéntico al chequeo por celda de Apotris (`tetrisEngine.cpp:34`), sin la rama `pawn.big`.
  - [x] Sin bucle de forma 4×4 ni `ActivePiece` — solo la celda única recibida por parámetro.

- [x] **Task 2: Prueba mínima en `main.c`** (AC: #4)
  - [x] Caso "ocupada": reutiliza `board_set(&gs, 3, 5, 7)` de la Story 2.1 → `board_is_cell_occupied(&gs, 3, 5)`.
  - [x] Caso "vacía": `(0, 0)`, nunca escrita.
  - [x] Caso "fuera de rango": `(-1, 0)`.
  - [x] Impreso en una sola línea: `consoleDrawText(1, 16, "COL OCC/EMPTY/OOB: %u %u %u", ...)` — fila 16, libre entre "PRESS A PAD BUTTON" (14) y el texto de pad (18).
  - [x] Loop principal, lectura de pad, BG0/BG1 y la prueba de board de la Story 2.1 sin cambios — esta prueba se agregó justo después, antes de `bgSetEnable(1)`/`setScreenOn()`.

## Dev Notes

- **Origen de la lógica (Apotris, solo lectura, líneas citadas):** `reference/apotris/source/tetrisEngine.cpp:17-52` (`Game::checkRotation`), condición de colisión por celda en la línea 34: `x + j < 0 || x + j > lengthX - 1 || y + i < 0 || y + i > lengthY - 1 || board[i+y][j+x] != 0`. `reference/apotris/include/tetrisEngine.h:356-358` confirma `lengthX=10`, `lengthY=40` (Apotris usa 40 filas por su modo "big"/2×2 escalado — **no aplica**, nuestro tablero ya es 22 filas fijas desde la Story 2.1, `game-architecture.md` §4). Solo se leyó esta función y estos campos, tal como pidió el usuario — no se analizó el resto de `tetrisEngine.cpp`/`.h`.
- **Qué se adapta y qué no:** el `deep-dive-tetris-core.md` ya clasifica esta función — 🟢 "núcleo (rama `!pawn.big`) es lógica de índices simple, casi copiable línea a línea" y 🔴 "rama `pawn.big` — eliminar, no dejar como dead code". Esta story toma solo el chequeo de **una celda** (borde + ocupada), sin el bucle de 4×4 sobre la forma de la pieza (`pawn.board[r][i][j]`) — ese bucle pertenece a la Story de movimiento/colisión de pieza en Epic 3, que no existe todavía porque no hay `ActivePiece`.
- **Tipos:** `s8`/`u8` de `snestypes.h` (vía `<snes.h>`, ya usado en `game_state.h`/`board.h` desde la Story 2.1). Sin floats, sin STL, sin alloc dinámica (`project-context.md`).
- Sin cambios a `board_get`/`board_set`/`board_clear`/`board_init` — `board_is_cell_occupied` es una función nueva que los reutiliza, no los reemplaza.

### References

- `reference/apotris/source/tetrisEngine.cpp:17-52` (única función leída de ese archivo)
- `reference/apotris/include/tetrisEngine.h:356-358` (única sección leída de ese archivo)
- `_bmad-output/deep-dive-tetris-core.md#Colisión`
- `_bmad-output/planning-artifacts/epics.md#Story 2.2: Colisión contra bordes y celdas ocupadas`
- `_bmad-output/game-architecture.md#4 Estructuras de datos principales`
- `_bmad-output/implementation-artifacts/2-1-tablero-logico.md` (`board.c`/`game_state.h` ya existentes, no se tocan salvo agregar la función nueva)

## Dev Agent Record

### Agent Model Used

claude-sonnet-5

### Debug Log References

- `make` en `snes/` (PVSNESLIB_HOME=~/Tools/pvsneslib): compiló y linkeó sin errores con `board_is_cell_occupied` agregada. `make clean` corrido al final.

### Completion Notes List

- Se leyó únicamente `reference/apotris/source/tetrisEngine.cpp:17-52` (`Game::checkRotation`) y `reference/apotris/include/tetrisEngine.h:356-358` (`lengthX`/`lengthY`/`board`), tal como pidió el usuario — nada más de esos archivos.
- La condición de `board_is_cell_occupied` es una traducción directa de la parte "por celda" de `checkRotation` (línea 34), sin el bucle 4×4 sobre `pawn.board[r][i][j]` (eso depende de una pieza/forma que no existe todavía) y sin la rama `pawn.big` (2×2 escalado, fuera de alcance del proyecto).
- Se agregó el resultado de la prueba en la fila 16 de la consola de texto (única fila libre entre las Stories anteriores: 10, 12, 14, 18).
- No se tocó `board_get`/`board_set`/`board_clear`/`board_init` ni ninguna otra Story.
- **No verificado por el agente:** resultado visual en emulador (AC #4) — no hay emulador en este entorno; pendiente de confirmación del usuario, igual que en las Stories anteriores.

### File List

- `snes/source/board.h` (modificado) — declaración de `board_is_cell_occupied`.
- `snes/source/board.c` (modificado) — implementación de `board_is_cell_occupied`; el resto del archivo sin cambios.
- `snes/source/main.c` (modificado) — prueba mínima de los 3 casos (ocupada/vacía/fuera de rango) impresa en fila 16, antes de `setScreenOn()`. Loop, pad, BG0/BG1 y prueba de la Story 2.1 sin cambios.

### Change Log

| Fecha | Cambio |
| --- | --- |
| 2026-07-11 | Implementación de Story 2.2: `board_is_cell_occupied` (bordes/piso + celda ocupada), adaptada de `Game::checkRotation` de Apotris. `make` compila sin errores. Status → in-progress (confirmación visual en emulador pendiente del usuario). |
