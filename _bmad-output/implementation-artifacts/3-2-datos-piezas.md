---
baseline_commit: 07be18e7f4c17c7908d3cb4a7a55ffbd54c1ed34
---

# Story 3.2: Tabla de formas de las 7 piezas (rotación 0)

Status: in-progress (implementación + build verificados; confirmación visual en emulador pendiente del usuario)

## Story

Como desarrollador,
quiero una tabla estática (`piece_data.h`/`piece_data.c`) con la forma de spawn (rotación 0) de las 7 piezas, adaptada de Apotris,
para tener el dato mínimo que futuras stories (spawn, render) necesitarán, sin todavía usarlo en ninguna lógica de juego.

## Alcance (fijado explícitamente por el usuario)

**Sí implementar:**
- `snes/source/piece_data.h` / `piece_data.c` — únicamente la tabla de formas de las 7 piezas en rotación 0.
- Prueba mínima en `main.c` (mismo patrón de las Stories 2.1/2.2/3.1): imprimir por consola un valor conocido de la tabla, para confirmar que compila y es accesible.

**NO implementar todavía:**
- Spawn (`piece_spawn()`, elegir tipo, colocar `ActivePiece` real) — story futura.
- Render/dibujo de la pieza (sprites, OAM — Epic 4).
- Movimiento de la pieza.
- Colisiones de pieza (usar la forma contra `board_is_cell_occupied` — story futura; hoy `board_is_cell_occupied` solo resuelve una celda, no una forma 4×4).
- Rotación, SRS, kicks (fuera de alcance del proyecto salvo pedido explícito).
- Cualquier otra tabla de Apotris (gravedad, scoring, finesse, `classic[]`/`ars[]`, colores) — no se leyeron ni se adaptan en esta story.

## Acceptance Criteria

1. **Given** `piece_data.h`/`piece_data.c`, **when** el proyecto compila, **then** existe una tabla estática con la forma de rotación 0 de las 7 piezas (grilla 4×4 por pieza, 0=vacío/1=ocupado).
2. **Given** la tabla ya definida, **when** se accede a una celda conocida (ej. la pieza I, fila 1, columna 0), **then** el valor coincide exactamente con el de Apotris (`tetraminos[0][0][1][0] == 1`).
3. **Given** el `main.c` existente, **when** arranca la ROM, **then** se imprime por consola de texto el valor de esa celda de prueba, confirmando que la tabla es accesible.
4. **Given** el resto del proyecto (`board.c`, `game_state.h`, `main.c`), **when** se revisa, **then** ningún módulo usa todavía la tabla para spawn, colisión de forma, movimiento o render — es solo el dato.
5. **Given** el proyecto compila con `make`, **when** se agrega `piece_data.c`, **then** no hay errores de compilación/link y las Stories anteriores (1.1, 1.2, 2.1, 2.2, 3.1) siguen funcionando sin cambios.

## Tasks / Subtasks

- [x] **Task 1: `piece_data.h`** (AC: #1)
  - [x] `PIECE_TYPE_COUNT 7` / `PIECE_GRID_SIZE 4`.
  - [x] `extern const u8 piece_shapes[PIECE_TYPE_COUNT][PIECE_GRID_SIZE][PIECE_GRID_SIZE];` — solo rotación 0.
  - [x] Sin funciones/lógica en el header — solo la declaración de la tabla.

- [x] **Task 2: `piece_data.c`** (AC: #1, #2)
  - [x] `piece_shapes[7][4][4]` definido con los 7 valores de rotación 0 (I/J/L/O/S/T/Z, mismo orden que `tetraminos[7][4][4][4]` de Apotris).
  - [x] Tipo `u8` (no `int`), sin bits de textura de borde.

- [x] **Task 3: Prueba mínima en `main.c`** (AC: #3, #4, #5)
  - [x] `#include "piece_data.h"` agregado.
  - [x] `consoleDrawText(1, 22, "PIECE DATA TEST: %u", piece_shapes[0][1][0])` — fila 22, libre (10/12/14/16/18/20 ya ocupadas por Stories anteriores).
  - [x] Loop principal, lectura de pad, BG0/BG1, `board.c` y pruebas de Stories 2.1/2.2/3.1 sin cambios.

## Dev Notes

- **Origen de los datos (Apotris, solo lectura, líneas citadas):** `reference/apotris/source/tetromino.cpp:19-177` (`GameInfo::tetraminos[7][4][4][4]`, solo índice de rotación 0 de cada uno de los 7 tipos). `reference/apotris/include/tetromino.hpp` (archivo completo, 59 líneas) — solo confirma la declaración `extern` de la tabla; el resto de tablas ahí declaradas (kicks, gravedad, scoring, finesse, `classic`/`ars`, colores) no se leyeron ni se usan.
- **Por qué solo rotación 0:** `project-context.md` ya lo fija explícitamente — "de `GameInfo::tetraminos[7][4][4][4]` (Apotris) solo se usa el índice de rotación 0 (forma de spawn)". No rotación system todavía, así que no hace falta cachear las 4 orientaciones.
- **Orden de tipos:** igual al de Apotris (`tetraminos[0..6]` = I, J, L, O, S, T, Z) — no se reordenó, para que sea trivial comparar contra la fuente si hace falta en el futuro.
- **Relación con `ActivePiece` (Story 3.1):** `ActivePiece.type` (agregado en la Story 3.1) es el índice que eventualmente indexará esta tabla — pero esta story no conecta ambos todavía (eso es spawn, story futura).
- Memoria estática, `const`, sin alloc dinámica (`project-context.md`). Módulo pequeño, una responsabilidad (solo datos), igual que `board.c`/`game_state.h`.

### References

- `reference/apotris/source/tetromino.cpp:19-177` (`tetraminos[7][4][4][4]`, solo rotación 0 de los 7 tipos)
- `reference/apotris/include/tetromino.hpp` (archivo completo, solo para confirmar la declaración `extern`)
- `_bmad-output/deep-dive-tetris-core.md#Pieza activa (Pawn)` (clasifica `tetraminos` como 🟢 reutilizable tal cual, y los bits de borde como omitibles para MVP)
- `_bmad-output/project-context.md#Referencia de código fuente (Apotris → mapeo)` ("solo el índice de rotación 0")
- `_bmad-output/game-architecture.md#1 Estructura mínima de archivos` (`piece_data.c/.h` ya previsto en el árbol de módulos)
- `_bmad-output/implementation-artifacts/3-1-estado-pieza-activa.md` (`ActivePiece.type`, no se toca en esta story)

## Dev Agent Record

### Agent Model Used

claude-sonnet-5

### Debug Log References

- `make` en `snes/` (PVSNESLIB_HOME=~/Tools/pvsneslib): compiló y linkeó sin errores con `piece_data.c` (nuevo) sumado al build. `make clean` corrido al final.

### Completion Notes List

- Tabla `piece_shapes[7][4][4]` es dato puro, sin funciones — ninguna lógica de spawn/render/movimiento/colisión de forma/rotación se agregó, tal como pide la Story.
- Valores tomados directamente de `reference/apotris/source/tetromino.cpp:19-177` (solo rotación 0 de cada una de las 7 piezas), sin releer `classic[]`/`ars[]`/kicks/gravedad/scoring.
- No se tocó `board.c`/`board.h`/`game_state.h` ni ninguna Story previa — solo se agregaron los 2 archivos nuevos y 2 líneas de include/prueba en `main.c`.
- **No verificado por el agente:** resultado visual en emulador (AC #3) — no hay emulador en este entorno; pendiente de confirmación del usuario, igual que en las Stories anteriores.

### File List

- `snes/source/piece_data.h` (nuevo) — declaración de `piece_shapes[7][4][4]`.
- `snes/source/piece_data.c` (nuevo) — definición de la tabla con las 7 formas de rotación 0.
- `snes/source/main.c` (modificado) — `#include "piece_data.h"` + prueba mínima en fila 22. Resto sin cambios.

### Change Log

| Fecha | Cambio |
| --- | --- |
| 2026-07-11 | Implementación de Story 3.2: `piece_data.h`/`.c` con la tabla de formas rotación-0 de las 7 piezas, adaptada de Apotris. `make` compila sin errores. Status → in-progress (confirmación visual en emulador pendiente del usuario). |
