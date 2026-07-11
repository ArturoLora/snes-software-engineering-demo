---
baseline_commit: f9edd9b9d6823753231ea642bbceadff3d6c018c
---

# Story 3.1: Estado de la pieza activa (ActivePiece)

Status: in-progress (implementación + build verificados; confirmación visual en emulador pendiente del usuario)

## Story

Como desarrollador,
quiero agregar un struct `ActivePiece` (tipo + posición) al `GameState` existente,
para tener el contrato de datos de la pieza activa antes de escribir spawn, movimiento o render (stories futuras).

## Alcance (fijado explícitamente por el usuario)

**Sí implementar:**
- Struct `ActivePiece` en `game_state.h`, agregado como campo de `GameState` (junto a `BoardState`, ya existente desde la Story 2.1).
- Campos mínimos: `type` (u8, 0-6, identifica cuál de las 7 piezas) y `x`, `y` (u8, posición en el tablero).
- Prueba mínima en `main.c` (mismo patrón de las Stories 2.1/2.2): escribir un valor de prueba en `gs.piece` e imprimirlo por consola, solo para confirmar que el struct existe y es accesible.

**NO implementar todavía:**
- Movimiento de la pieza (ningún código que cambie `x`/`y` en runtime).
- Render/dibujo de la pieza (sprites, OAM — eso es Epic 4).
- Spawn (`piece_spawn()`, elegir tipo, colocar en posición inicial real — Story futura).
- Rotación, SRS, kicks (fuera de alcance de todo el proyecto salvo pedido explícito, `project-context.md`).
- Gravedad, lock, line clear, cola/7-bag, scoring.
- Tabla de formas (`piece_data.c`, rotación-0 de las 7 piezas) — esta story NO la crea; `ActivePiece.type` es solo un índice sin tabla asociada todavía. La tabla de formas es una story futura separada (orden de `game-architecture.md` §10: `game_state.h` → `board.c` → `piece_data.c` → `piece.c`).

## Acceptance Criteria

1. **Given** `game_state.h`, **when** se agrega el struct `ActivePiece { u8 type; u8 rotation; s8 x; s8 y; }`, **then** `GameState` lo incluye como campo nuevo (ej. `ActivePiece piece;`) sin romper el campo `board` ya existente.
2. **Given** el `main.c` existente, **when** arranca la ROM, **then** se escribe un valor de prueba en `gs.piece.type`/`gs.piece.x`/`gs.piece.y` y se imprime por consola de texto, confirmando que el struct compila y es accesible.
3. **Given** el resto del código del proyecto (`board.c`, `board.h`), **when** se revisa, **then** ningún archivo fuera de `main.c` (prueba) escribe `gs.piece` todavía — no existe lógica de pieza real, solo el dato.
4. **Given** el proyecto compila con `make`, **when** se agrega `ActivePiece`, **then** no hay errores de compilación/link y el resto de las Stories (1.1, 1.2, 2.1, 2.2) siguen funcionando sin cambios.

## Tasks / Subtasks

- [x] **Task 1: `ActivePiece` en `game_state.h`** (AC: #1)
  - [x] `typedef struct { u8 type; u8 rotation; s8 x; s8 y; } ActivePiece;` implementado tal cual.
  - [x] `ActivePiece piece;` agregado a `GameState`, junto al campo `board` ya existente. `game_state.h` sigue siendo solo structs, sin lógica.

- [x] **Task 2: Prueba mínima en `main.c`** (AC: #2, #4)
  - [x] `gs.piece.type = 0; gs.piece.rotation = 0; gs.piece.x = 4; gs.piece.y = 0;` (valores de prueba, no un spawn real).
  - [x] Impreso con `consoleDrawText(1, 20, "PIECE TEST: %u %u %d %d", ...)` — fila 20, libre (10/12/14/16/18 ya ocupadas por Stories 1.1/1.2/2.1/2.2).
  - [x] Loop principal, lectura de pad, BG0/BG1 y pruebas de board/colisión existentes sin cambios.

## Dev Notes

- **Origen de la lógica (Apotris, solo lectura, archivos y líneas citadas):**
  - `reference/apotris/include/tetromino.hpp` (archivo completo, 59 líneas) — solo declaraciones `extern` de tablas de datos (`tetraminos[7][4][4][4]`, `colors[7][3]`, gravedad, scoring, kicks, etc.). No hay struct de pieza activa acá — solo tablas.
  - `reference/apotris/source/tetromino.cpp:9-17` (`colors[7][3]`) y `tetromino.cpp:19-104` (`tetraminos[7][4][4][4]`, primeras 3 piezas de 7): cada tipo de pieza tiene 4 rotaciones, cada una una grilla 4×4 de `int` (0=vacío, 1=ocupado). La forma de spawn es el índice de rotación 0 (`tetraminos[type][0][][]`).
  - **No se leyó** `tetrisEngine.h`/`tetrisEngine.cpp` en esta story (ya se leyó `Pawn`/`checkRotation` en la Story 2.2) — el struct `Pawn` real de Apotris (`x,y,type,current,rotation,board[4][4][4],lowest,big`) no se repite acá; se toma como referencia conceptual, no se lee de nuevo.
- **Por qué `type`/`rotation`/`x`/`y`:** `game-architecture.md` §4 ya definía `ActivePiece` como `type u8 (0-6) · x,y u8 · referencia a la forma rotación-0 en piece_data (no se copia)` — esta story ajusta esa base agregando `rotation` (u8, índice de rotación, siempre 0 mientras no exista sistema de rotación) y usando `x`/`y` como `s8` en vez de `u8` (necesario para futuras validaciones de colisión con coordenadas negativas, igual que en `board_is_cell_occupied` de la Story 2.2). Sin los campos de Apotris que no aplican (`big` es del modo 2×2 escalado, fuera de alcance; `lowest`/`current` son de lock-timer/color, stories futuras).
- **Sin tabla de formas todavía:** `ActivePiece.type` es un índice (0-6) sin ninguna tabla que lo resuelva en esta story — `piece_data.c` (rotación-0 de las 7 piezas, adaptado de `tetraminos[7][0][4][4]`) es explícitamente la siguiente story, no esta.
- Memoria estática, sin alloc dinámica (`project-context.md`) — `ActivePiece` vive dentro de la misma `GameState gs` estática ya declarada en `main.c` desde la Story 2.1.

### References

- `reference/apotris/include/tetromino.hpp` (archivo completo)
- `reference/apotris/source/tetromino.cpp:9-104` (`colors`, `tetraminos`, primeras 3 piezas)
- `_bmad-output/game-architecture.md#4 Estructuras de datos principales`, `#10 Orden recomendado de implementación`
- `_bmad-output/project-context.md#Alcance mínimo, Anti-patrones para agentes`
- `_bmad-output/planning-artifacts/epics.md#Story 3.1: Tabla de formas y spawn de pieza` (nota: epics.md agrupa tabla+spawn en una sola story; esta Story 3.1 del usuario es más chica — solo el struct `ActivePiece`, sin tabla ni spawn, que quedan para stories siguientes)
- `_bmad-output/implementation-artifacts/2-1-tablero-logico.md`, `2-2-colision-basica.md` (`GameState`/`board.c` ya existentes, no se tocan salvo agregar el campo `piece`)

## Dev Agent Record

### Agent Model Used

claude-sonnet-5

### Debug Log References

- `make` en `snes/` (PVSNESLIB_HOME=~/Tools/pvsneslib): compiló y linkeó sin errores con `ActivePiece` agregado a `GameState`. `make clean` corrido al final.

### Completion Notes List

- Sin tablas de piezas, sin spawn, sin render, sin movimiento, sin rotación real — `ActivePiece` es solo el dato (`type`/`rotation`/`x`/`y`), tal como pide la Story.
- `%d` usado para `s8 x`/`s8 y` en `consoleDrawText` (se promueven a `int` como cualquier argumento variádico en C) — mismo patrón printf-style ya confirmado en Stories anteriores.
- No se tocó `board.c`/`board.h` ni ninguna otra Story — solo `game_state.h` (nuevo struct + campo) y `main.c` (prueba nueva en fila 20).
- **No verificado por el agente:** resultado visual en emulador (AC #2) — no hay emulador en este entorno; pendiente de confirmación del usuario, igual que en las Stories anteriores.

### File List

- `snes/source/game_state.h` (modificado) — `ActivePiece{type,rotation,x,y}` + campo `piece` en `GameState`.
- `snes/source/main.c` (modificado) — prueba mínima de `ActivePiece` en fila 20, antes de `setScreenOn()`. Resto sin cambios.

### Change Log

| Fecha | Cambio |
| --- | --- |
| 2026-07-11 | Implementación de Story 3.1: `ActivePiece{type,rotation,x,y}` agregado a `GameState`, prueba mínima en `main.c`. `make` compila sin errores. Status → in-progress (confirmación visual en emulador pendiente del usuario). |
