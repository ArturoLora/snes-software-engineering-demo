---
baseline_commit: 0a3fff208dbf31e4f7793cb56ab917ae56779feb
---

# Story 2.1: Estructuras de estado base y tablero lógico

Status: in-progress (implementación + build verificados; confirmación visual en emulador pendiente del usuario)

## Story

Como desarrollador,
quiero definir `game_state.h` y un `board.c` con el tablero estático (22×10) inicializado en ceros y funciones básicas de acceso,
para tener el contrato de datos entre módulos y un tablero lógico verificable sin hardware de video, antes de escribir cualquier lógica de pieza.

## Alcance (fijado explícitamente por el usuario)

**Sí implementar:**
- `GameState` (en `game_state.h`) — por ahora solo agrupa el tablero; sin structs de pieza/cola/gravedad todavía (se agregan en stories futuras, no antes).
- `board[22][10]` (`u8`) — 20 filas visibles + 2 de buffer superior, tal como define `game-architecture.md` §4.
- Inicialización del tablero (`board_init()`).
- `board_clear()` — pone todas las celdas en 0 (vacío).
- `board_get(x, y)` — lee una celda.
- `board_set(x, y, value)` — escribe una celda.
- Prueba mínima desde `main.c`: llamar `board_init()`, `board_set()` sobre una celda, `board_get()` esa misma celda, e imprimir el resultado por la consola de texto ya existente (Story 1.1) para verificar que la estructura existe y funciona.

**NO implementar en esta story** (aunque `game-architecture.md` las describa para stories futuras): piezas, spawn, gravedad, colisiones, render, line clear, cola/7-bag. `board.c` no conoce nada de eso todavía — es solo el dato `board[][]` y sus 3 funciones de acceso.

## Acceptance Criteria

1. **Given** el proyecto compila con `game_state.h` y `board.c`/`board.h`, **when** arranca la ROM, **then** `board_init()` deja las 22×10 celdas en 0.
2. **Given** el tablero ya inicializado, **when** se llama `board_set(x, y, valor)` sobre una celda dentro de rango, **then** `board_get(x, y)` esa misma celda devuelve `valor`.
3. **Given** el tablero con celdas ya escritas, **when** se llama `board_clear()`, **then** todas las celdas vuelven a 0.
4. **Given** el `main.c` existente (consola de texto de la Story 1.1), **when** arranca la ROM, **then** se imprime por consola de texto la prueba mínima (ej. valor escrito y valor leído de una celda de prueba), confirmando visualmente que `board_get`/`board_set` funcionan.
5. **Given** el resto de los módulos del proyecto (`main.c`), **when** se revisa el código, **then** ningún módulo fuera de `board.c` escribe directamente el array `board[][]` — todo acceso pasa por `board_get`/`board_set`.

## Tasks / Subtasks

- [x] **Task 1: `snes/source/game_state.h`** (AC: #1)
  - [x] `BoardState { u8 board[22][10]; }` (`BOARD_WIDTH`/`BOARD_HEIGHT` = 10/22, tipo `u8` vía `<snes.h>`).
  - [x] `GameState { BoardState board; }` — único campo por ahora; sin structs de pieza/cola/gravedad.
  - [x] Sin lógica en el header — solo structs y las 2 constantes de tamaño.

- [x] **Task 2: `snes/source/board.c` / `board.h`** (AC: #1, #2, #3, #5)
  - [x] `void board_init(GameState *gs)` — llama a `board_clear`.
  - [x] `void board_clear(GameState *gs)` — pone las 22×10 celdas en 0.
  - [x] `u8 board_get(GameState *gs, u8 x, u8 y)` — lee `board[y][x]`; fuera de rango devuelve 0.
  - [x] `void board_set(GameState *gs, u8 x, u8 y, u8 value)` — escribe `board[y][x]`; fuera de rango no hace nada.
  - [x] `board.c` es el único módulo que toca `board[][]` directamente.

- [x] **Task 3: Prueba mínima en `main.c`** (AC: #4)
  - [x] `static GameState gs;` en `main.c` (memoria estática, sin alloc dinámica).
  - [x] Antes de `setScreenOn()`: `board_init(&gs)`, `board_set(&gs, 3, 5, 7)`, `board_get(&gs, 3, 5)`.
  - [x] Impreso con `consoleDrawText(3, 12, "BOARD TEST: %u", board_get(&gs, 3, 5))` — fila 12, libre entre "Hello Apotris SNES" (fila 10) y "PRESS A PAD BUTTON" (fila 14). `consoleDrawText` acepta formato tipo printf (confirmado localmente en `snes-examples/random/random.c` y `snes-examples/timer/timer.c`, que usan `%04x`/`%u`).
  - [x] Loop principal, lectura de pad y BG1/playfield de las Stories 1.1/1.2 sin cambios — la prueba se agregó antes de `bgSetEnable(1)`/`setScreenOn()`.

## Dev Notes

- **Sin lógica de colisión todavía:** el guard de límites en `board_get`/`board_set` es solo para que escribir/leer fuera de rango no corrompa memoria — no es `board_is_cell_occupied` (esa función, para colisión real de piezas, es la Story 2.2 de `epics.md`).
- **Sin render del tablero:** esta story no toca BG1 (ya inicializado como playfield vacío en la Story 1.2) ni ningún dato de VRAM. `board.c` no incluye headers de video, tal como exige `game-architecture.md` §7 ("lógica de gameplay sin dependencia de headers de video de PVSnesLib").
- **Tamaño del tablero confirmado por `game-architecture.md` §4:** `board[22][10]` — 20 filas visibles + 2 de buffer superior para spawn/game-over (no 40 filas como en Apotris original, que tenía big-mode; ver `deep-dive-tetris-core.md`, fuera del alcance de esta story).
- Memoria estática: `GameState` vive como variable estática/global en `main.c` (o `static` a nivel de archivo) — nada de `malloc`/`new` (`project-context.md`, NFR1 de `epics.md`).
- Módulos C pequeños, un archivo por responsabilidad (`project-context.md`) — esta story agrega solo `game_state.h` y `board.c`/`.h`; no crear `piece.c`/`queue.c`/`render.c` todavía.

### References

- `_bmad-output/game-architecture.md#3 Ownership del estado`, `#4 Estructuras de datos principales`, `#7 Separación gameplay/render`
- `_bmad-output/planning-artifacts/epics.md#Story 2.1: Estructuras de estado base y tablero vacío verificable`
- `_bmad-output/project-context.md#Alcance mínimo, Defaults de implementación, Anti-patrones para agentes`
- `_bmad-output/implementation-artifacts/1-1-boot-infraestructura-minima.md`, `1-2-playfield-vacio.md` (consola de texto y BG1 ya existentes, no se tocan)

## Dev Agent Record

### Agent Model Used

claude-sonnet-5

### Debug Log References

- `make` en `snes/` (PVSNESLIB_HOME=~/Tools/pvsneslib): compiló y linkeó `board.c` (nuevo) sin errores junto al resto del proyecto. `make clean` corrido al final.

### Completion Notes List

- **Sin cambios a las Stories 1.1/1.2:** solo se agregaron 2 `#include`, la variable `static GameState gs;`, y 3 líneas de prueba antes de `bgSetEnable(1)`/`setScreenOn()` en `main.c`. El loop, la lectura de pad y el init de BG0/BG1 quedaron intactos.
- **`consoleDrawText` es printf-style** (`char *fmt, ...`, confirmado en `console.h` y en el uso real de `random.c`/`timer.c`) — no hizo falta ninguna conversión manual de entero a string para la prueba.
- **Guard de límites en `board_get`/`board_set`:** simple `if (x >= BOARD_WIDTH || y >= BOARD_HEIGHT) return;` — solo evita corromper memoria fuera del array; no es un sistema de colisión (eso es la Story 2.2, fuera de este alcance).
- **No verificado por el agente:** el resultado visual en emulador (AC #4, "se imprime BOARD TEST: 7" en pantalla) — no hay emulador en este entorno. Pendiente de que el usuario lo confirme como en las Stories anteriores.
- AC #5 (ningún otro módulo escribe `board[][]` directamente) verificado por inspección: `main.c` solo llama a `board_init`/`board_set`/`board_get`, nunca indexa `gs.board.board` directamente.

### File List

- `snes/source/game_state.h` (nuevo) — `BoardState`/`GameState`.
- `snes/source/board.h` (nuevo) — declaraciones de `board_init`/`board_clear`/`board_get`/`board_set`.
- `snes/source/board.c` (nuevo) — implementación; único módulo que escribe `board[][]`.
- `snes/source/main.c` (modificado) — `#include "game_state.h"`/`"board.h"`, `static GameState gs;`, prueba mínima de board antes de `setScreenOn()`. Loop, pad y BG0/BG1 sin cambios.

### Change Log

| Fecha | Cambio |
| --- | --- |
| 2026-07-11 | Implementación de Story 2.1: tablero lógico estático (`game_state.h`+`board.c`) con `init`/`clear`/`get`/`set`, prueba mínima en `main.c`. `make` compila sin errores. Status → in-progress (confirmación visual en emulador pendiente del usuario). |
