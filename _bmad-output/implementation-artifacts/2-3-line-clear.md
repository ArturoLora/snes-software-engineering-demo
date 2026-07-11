---
baseline_commit: f7dbd18937f5044858ddba5b1b2fe9baa637eba7
---

# Story 2.3: Detección y colapso de líneas completas

Status: in-progress (implementación + build verificados; confirmación visual en emulador pendiente del usuario)

## Story

Como desarrollador,
quiero detectar filas completas del tablero y colapsarlas (shift-down) de inmediato,
para tener la regla de line-clear lista antes de conectarla al ciclo real de la pieza activa.

## Nota de roadmap

Esta story es la **Story 2.3 original de `epics.md`** (Epic 2: Tablero y colisión), no una continuación de la numeración `3.x` usada en las stories recientes (Epic 3: ciclo de la pieza activa). Según el roadmap de `epics.md`, `2.3` va inmediatamente después de `2.2` (colisión) y antes de todo Epic 3 — quedó pendiente. Esta story la completa, **sin reordenar el roadmap y sin integrarla todavía al ciclo de la pieza** (esa conexión es la Story 3.6 original de `epics.md`, "Line clear integrado al ciclo de lock", que sigue siendo una story futura separada).

## Alcance (derivado de `epics.md` Story 2.3 + `game-architecture.md` §4)

**Sí implementar:**
- Agregar `LinesToClear { u8 rows[4]; u8 count; }` a `game_state.h` (ya previsto en `game-architecture.md` §4: "máx. 4 líneas simultáneas") y un campo `LinesToClear lines;` en `GameState`.
- Ampliar `snes/source/board.c`/`board.h` con:
  - `u8 board_detect_full_lines(GameState *gs)` — recorre las `BOARD_HEIGHT` filas; si una fila tiene las `BOARD_WIDTH` columnas ocupadas (`board_get != 0`), la agrega a `gs->lines.rows[]` (máx. 4) e incrementa `gs->lines.count`. Devuelve `gs->lines.count`.
  - `void board_collapse_lines(GameState *gs)` — para cada fila registrada en `gs->lines.rows[]` (en el mismo orden en que se detectaron, de arriba hacia abajo), desplaza las filas superiores una posición hacia abajo (`board[j][k] = board[j-1][k]` para `j` desde la fila hasta 1); al final, resetea `gs->lines.count = 0`. Colapso inmediato, sin delay de animación (tal como pide `epics.md`).
- Prueba por consola: rellenar una fila de prueba completa a mano (`board_set` celda por celda), llamar `board_detect_full_lines()`, llamar `board_collapse_lines()`, verificar con `board_get()` que la fila se colapsó correctamente.

**NO implementar todavía:**
- Conexión con `piece_lock()` / ciclo de la pieza activa (eso es la Story 3.6 original — story futura separada).
- T-spin, scoring, combo, DIG/COMBO/zone (Apotris los mezcla en la misma función, acá se excluyen).
- Render/animación de line-clear.
- Múltiples filas no contiguas con solapamiento complejo más allá de lo que ya cubre el algoritmo adaptado (máx. 4 simultáneas, igual que Apotris).

## Acceptance Criteria

1. **Given** un tablero con una fila de prueba completa (rellenada por debug con `board_set`), **when** se llama `board_detect_full_lines(gs)`, **then** devuelve `1` y `gs->lines.rows[0]` contiene el índice de esa fila.
2. **Given** un tablero sin filas completas, **when** se llama `board_detect_full_lines(gs)`, **then** devuelve `0`.
3. **Given** una fila completa detectada, **when** se llama `board_collapse_lines(gs)`, **then** las filas superiores a la fila colapsada bajan una posición (verificable comparando una celda de referencia antes/después con `board_get()`), y `gs->lines.count` vuelve a `0`.
4. **Given** el `main.c` existente, **when** arranca la ROM, **then** se imprime por consola de texto el resultado de la detección y del colapso, confirmando visualmente el comportamiento.
5. **Given** el proyecto compila con `make`, **when** se agregan las 2 funciones nuevas y `LinesToClear`, **then** no hay errores de compilación/link y las Stories anteriores (1.1 a 3.6) siguen funcionando sin cambios.

## Tasks / Subtasks

- [x] **Task 1: `LinesToClear` en `game_state.h`** (AC: #1, #3)
  - [x] `typedef struct { u8 rows[LINES_TO_CLEAR_MAX]; u8 count; } LinesToClear;` (`LINES_TO_CLEAR_MAX 4`).
  - [x] `LinesToClear lines;` agregado a `GameState`, junto a `board`/`piece`.

- [x] **Task 2: `board_detect_full_lines`/`board_collapse_lines` en `board.h`/`board.c`** (AC: #1, #2, #3)
  - [x] `board_detect_full_lines`: doble bucle fila/columna vía `board_get()`, tope de `LINES_TO_CLEAR_MAX` filas.
  - [x] `board_collapse_lines`: shift-down vía `board_get`/`board_set`, resetea `gs->lines.count = 0` al final.
  - [x] Sin T-spin, scoring, `fixConnected`, `zoneTimer`/DIG/COMBO, sin llamada a `next()`.
  - [x] Nota heredada de Apotris (fila 0 nunca sobreescrita) documentada en comentario del código.

- [x] **Task 3: Prueba mínima en `main.c`** (AC: #4, #5)
  - [x] Marcador de referencia en `(0,9)` valor `9`, fila 10 rellenada completa con valor `5` — sin pisar `(3,5)`.
  - [x] `board_detect_full_lines(&gs)` impreso en fila 18: `"LINES DET:%u ROW0:%u"`.
  - [x] `board_collapse_lines(&gs)` impreso en fila 20: `"LINES COLLAPSE ROW10:%u CNT:%u"` — verifica que el marcador `9` bajó a `(0,10)` y que `count` volvió a 0.
  - [x] Todos los valores nuevos casteados a `s16`/`u16`.
  - [x] Loop principal, pad, BG0/BG1, `piece.c`, `piece_data.c` y pruebas existentes (filas 2, 4, 6, 8, 10, 12, 14, pad en fila 16) sin cambios.

## Dev Notes

- **Origen de la lógica (Apotris, solo lectura, líneas citadas):** `reference/apotris/source/tetrisEngine.cpp:814-886` (detección, dentro de `Game::clear()` — solo el bucle de filas/columnas, líneas 870-886; no se leyó el chequeo de T-spin de líneas 823-862 ni el resto de `clear()`) y `tetrisEngine.cpp:1624-1672` (`Game::removeClearLock()` — solo el bucle de shift-down, líneas 1624-1639; no se leyó la rama `gameMode==COMBO` ni la llamada a `next()`).
- **Por qué "de inmediato" y no diferido:** Apotris difiere el colapso real vía `clearLock`/`removeClearLock()` (para animación). `epics.md` Story 2.3 pide explícitamente "el colapso ocurre en el mismo paso, sin delay de animación" — esta story colapsa inmediatamente tras detectar, sin estado `clearLock` intermedio.
- **Ownership:** `board_detect_full_lines`/`board_collapse_lines` viven en `board.c`, que sigue siendo el único módulo que toca `board[][]` directamente (vía `board_get`/`board_set` internos, no acceso crudo). `game_state.h` sigue siendo solo structs.
- Sin cambios a `piece.c` (`piece_spawn`/`piece_move_left`/`piece_move_right`/`piece_apply_gravity`/`piece_lock`), `piece_data.c`, ni a `board_init`/`board_get`/`board_set`/`board_is_cell_occupied` ya existentes.
- Memoria estática, sin alloc dinámica, C puro (`project-context.md`).

### References

- `reference/apotris/source/tetrisEngine.cpp:870-886` (detección de filas completas, dentro de `Game::clear()`)
- `reference/apotris/source/tetrisEngine.cpp:1624-1639` (shift-down, dentro de `Game::removeClearLock()`)
- `_bmad-output/planning-artifacts/epics.md#Story 2.3: Detección y colapso de líneas completas` (story original de este roadmap)
- `_bmad-output/game-architecture.md#4 Estructuras de datos principales` (`LinesToClear` ya especificada: `rows[4]`, `count`, máx. 4 líneas)
- `_bmad-output/project-context.md#Alcance mínimo, Anti-patrones para agentes`
- `_bmad-output/implementation-artifacts/2-1-tablero-logico.md`, `2-2-colision-basica.md` (`board_get`/`board_set`/`board_is_cell_occupied` ya existentes, no se tocan)

## Dev Agent Record

### Agent Model Used

claude-sonnet-5

### Debug Log References

- `make` en `snes/` (PVSNESLIB_HOME=~/Tools/pvsneslib): compiló y linkeó sin errores con `board_detect_full_lines`/`board_collapse_lines` sumadas a `board.c` y `LinesToClear` a `game_state.h`. `make clean` corrido al final.

### Completion Notes List

- Story 2.3 original de `epics.md` (Epic 2), no una continuación de la numeración `3.x` — se implementó sin reordenar el roadmap ni conectar con `piece_lock()`/spawn/render/scoring/combo/top-out, tal como se pidió.
- Diseño de prueba: marcador `9` en `(0,9)` (fila justo arriba de la fila a limpiar) + fila 10 rellenada por completo con `5`. Tras `board_collapse_lines()`, el marcador debe aparecer en `(0,10)` — confirma que la fila superior bajó una posición, sin necesidad de inspeccionar el array crudo.
- `board_detect_full_lines`/`board_collapse_lines` usan `board_get`/`board_set` internamente — `board.c` sigue siendo el único módulo que toca `gs->board.board` directamente.
- No se tocó `piece.c` (`piece_spawn`/`piece_move_left`/`piece_move_right`/`piece_apply_gravity`/`piece_lock`), `piece_data.c`, ni `board_init`/`board_get`/`board_set`/`board_is_cell_occupied` ya existentes.
- **No verificado por el agente:** resultado visual en emulador (AC #4) — no hay emulador en este entorno; pendiente de confirmación del usuario, igual que en las Stories anteriores.

### File List

- `snes/source/game_state.h` (modificado) — `LinesToClear{rows[4],count}` + campo `lines` en `GameState`.
- `snes/source/board.h` (modificado) — declaraciones de `board_detect_full_lines`/`board_collapse_lines`.
- `snes/source/board.c` (modificado) — implementación de ambas; `board_init`/`board_clear`/`board_get`/`board_set`/`board_is_cell_occupied` sin cambios.
- `snes/source/main.c` (modificado) — prueba mínima en filas 18/20. Resto sin cambios.

### Change Log

| Fecha | Cambio |
| --- | --- |
| 2026-07-11 | Implementación de Story 2.3 (roadmap original, Epic 2): `board_detect_full_lines`/`board_collapse_lines` en `board.c`, `LinesToClear` en `game_state.h`, adaptados de `Game::clear()`/`Game::removeClearLock()` de Apotris. Colapso inmediato, sin conexión a `piece_lock()` todavía. `make` compila sin errores. Status → in-progress (confirmación visual en emulador pendiente del usuario). |
