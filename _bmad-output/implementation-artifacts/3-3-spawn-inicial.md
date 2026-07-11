---
baseline_commit: 80ce5b7b0d0e7f540690dcde446de7b7d6a83097
---

# Story 3.3: Spawn inicial de la pieza activa

Status: in-progress (implementación + build verificados; confirmación visual en emulador pendiente del usuario)

## Story

Como desarrollador,
quiero una función `piece_spawn()` que inicialice `ActivePiece` con una pieza fija en su posición inicial,
para tener el primer estado de pieza activa verificable, antes de que exista cola/7-bag, movimiento o gravedad.

## Alcance (fijado explícitamente por el usuario)

**Sí implementar:**
- `snes/source/piece.h` / `piece.c` (primer módulo de este archivo, según el orden de `game-architecture.md` §10) con `void piece_spawn(GameState *gs)`.
- `piece_spawn()` inicializa `gs->piece`: `type = 0` (pieza fija — todavía no existe cola/7-bag), `rotation = 0`, y posición inicial (`x`, `y`).
- Prueba mínima en `main.c` (mismo patrón de las Stories 2.1/2.2/3.1/3.2): llamar `piece_spawn(&gs)` e imprimir el estado resultante por consola.

**NO implementar todavía:**
- Cola/7-bag (`queue.c`) — `type` queda hardcodeado a 0 hasta que exista.
- Render de la pieza (sprites, OAM — Epic 4).
- Movimiento de la pieza (horizontal, DAS/ARR).
- Gravedad.
- Colisiones de pieza (chequeo de la forma 4×4 contra `board_is_cell_occupied`, o el chequeo de top-out en las filas de buffer — ambos se difieren a stories futuras, ej. la Story de lock, que en Apotris es donde vive la condición de derrota).
- Lock.

## Acceptance Criteria

1. **Given** `GameState` con un `ActivePiece` sin inicializar, **when** se llama `piece_spawn(gs)`, **then** `gs->piece.type == 0`, `gs->piece.rotation == 0`.
2. **Given** la misma llamada, **when** se inspecciona la posición resultante, **then** `gs->piece.x == BOARD_WIDTH/2 - 2` (3) y `gs->piece.y == 2` — posición análoga a la de Apotris (`lengthX/2-2`, y en el borde buffer/visible del tablero), adaptada a nuestro `board[22][10]` (2 filas de buffer, la Story 2.1 ya las reservó).
3. **Given** el `main.c` existente, **when** arranca la ROM, **then** se llama `piece_spawn(&gs)` y se imprime por consola de texto el estado resultante (`type`, `rotation`, `x`, `y`), confirmando visualmente el spawn.
4. **Given** el resto del proyecto (`board.c`, `piece_data.c`), **when** se revisa el código, **then** `piece_spawn()` no llama a `board_is_cell_occupied` ni a ninguna función de colisión/lock — solo escribe `gs->piece`.
5. **Given** el proyecto compila con `make`, **when** se agrega `piece.c`, **then** no hay errores de compilación/link y las Stories anteriores (1.1 a 3.2) siguen funcionando sin cambios.

## Tasks / Subtasks

- [x] **Task 1: `piece.h`** (AC: #1, #2)
  - [x] `void piece_spawn(GameState *gs);` declarado.
  - [x] Incluye `game_state.h`; sin headers de video ni `piece_data.h` (no hace falta todavía).

- [x] **Task 2: `piece.c`** (AC: #1, #2, #4)
  - [x] `piece_spawn(GameState *gs)`: `type=0` (fijo, comentado como temporal hasta `queue.c`), `rotation=0`, `x=BOARD_WIDTH/2-2` (3), `y=2` (borde buffer/visible, no `BOARD_HEIGHT/2`).
  - [x] Sin `pawn.setBlock()`/modo CLASSIC/`pawn.big`/top-out/hold/ARR — nada de eso se portó.

- [x] **Task 3: Prueba mínima en `main.c`** (AC: #3, #5)
  - [x] `#include "piece.h"` agregado.
  - [x] Los 4 valores hardcodeados de la Story 3.1 se reemplazaron por `piece_spawn(&gs);` real.
  - [x] Misma impresión en fila 20 (`consoleDrawText(1, 20, "PIECE TEST: %u %u %d %d", ...)`), ahora con el resultado del spawn real.
  - [x] Loop principal, pad, BG0/BG1, `board.c`, `piece_data.c` y pruebas existentes (filas 12, 16, 22) sin cambios.

## Dev Notes

- **Origen de la lógica (Apotris, solo lectura, líneas citadas):** `reference/apotris/source/tetrisEngine.cpp:1202-1274` (`Game::next()`). Solo se leyó esta función — no se leyó `fillBag()`/`fillQueue()` (cola/7-bag, fuera de alcance) ni ninguna otra parte de `tetrisEngine.cpp`/`.h`.
  - Posición inicial: `pawn.y = lengthY/2` (línea 1203), `pawn.x = lengthX/2-2` (línea 1204).
  - Rotación: `pawn.rotation = 0` en el caso general — la rama `irs`/`ARS` (líneas 1206-1211) es de otros sistemas de rotación (fuera de alcance del proyecto).
  - Selección de pieza: `pawn.current = queue.front(); queue.pop_front(); fillQueue(1)` (líneas 1215-1217) — depende de `queue`/7-bag (`fillBag()`/`fillQueue()`, no leídas, fuera de alcance). Reemplazado temporalmente por `type = 0` fijo, tal como pediste.
  - Resto de la función (modo CLASSIC líneas 1224-1230, `pawn.big` líneas 1232-1235, chequeo de top-out en filas 21-23 líneas 1238-1261, flags de movimiento/hold líneas 1263-1273) — colisión/game-over/hold/modos de juego, explícitamente fuera de esta story.
- **Por qué `y = 2` y no `BOARD_HEIGHT/2`:** Apotris spawnea justo en el límite entre buffer superior y área visible (`lengthY/2` = 20 de 40, el primer índice del área visible). Nuestro tablero (`board[22][10]`, Story 2.1) tiene esa misma proporción de buffer pero en 2 filas, no 20 — el spawn análogo es la fila 2, no una división por 2 del total (que daría 11 y caería en medio del área visible, no en el borde del buffer).
- **Relación con `piece_data.c` (Story 3.2):** esta story NO usa todavía `piece_shapes` — `piece_spawn()` solo fija `type` (el índice), sin resolver la forma. Conectar `type` con `piece_shapes[type]` es de una story futura (colisión de forma / render).
- Memoria estática, sin alloc dinámica (`project-context.md`). Módulo nuevo pequeño, una responsabilidad (`piece.c` = dueño de `ActivePiece`, igual que `board.c` es dueño de `BoardState` desde la Story 2.1 — ownership de `game-architecture.md` §2/§3).

### References

- `reference/apotris/source/tetrisEngine.cpp:1202-1274` (`Game::next()`, única función leída de ese archivo)
- `_bmad-output/game-architecture.md#2 Módulos y responsabilidad`, `#3 Ownership del estado`, `#4 Estructuras de datos principales`, `#10 Orden recomendado de implementación`
- `_bmad-output/project-context.md#Alcance mínimo, Anti-patrones para agentes`
- `_bmad-output/planning-artifacts/epics.md#Story 3.1: Tabla de formas y spawn de pieza` (epics.md agrupa tabla+spawn+queue en una story; el usuario lo dividió en 3.1 ActivePiece / 3.2 piece_data / 3.3 spawn — esta story es solo la última parte, con pieza fija en vez de cola real)
- `_bmad-output/implementation-artifacts/3-1-estado-pieza-activa.md`, `3-2-datos-piezas.md` (`ActivePiece`/`piece_shapes` ya existentes, no se tocan)

## Dev Agent Record

### Agent Model Used

claude-sonnet-5

### Debug Log References

- `make` en `snes/` (PVSNESLIB_HOME=~/Tools/pvsneslib): compiló y linkeó sin errores con `piece.c` (nuevo, primer módulo `piece.c`) sumado al build. `make clean` corrido al final.

### Completion Notes List

- `piece_spawn()` es la primera función que escribe `gs->piece` (antes lo hacía `main.c` a mano, Story 3.1) — ownership consistente con `board.c` para `BoardState`.
- Sin colisión/top-out, sin render, sin movimiento/gravedad, sin lock, sin 7-bag — `piece_spawn()` solo asigna 4 campos, tal como pide la Story.
- `y=2` en vez de una fórmula `BOARD_HEIGHT/2`: justificado en Dev Notes de la Story — Apotris spawnea en el borde buffer/visible (proporcional), no en la mitad geométrica del tablero total.
- No se tocó `board.c`/`board.h`/`piece_data.c`/`piece_data.h`/`game_state.h` ni ninguna Story previa — solo se agregaron `piece.c`/`piece.h` y se reemplazó la prueba manual de la Story 3.1 en `main.c` por la llamada real.

**Fix post-validación visual (2026-07-11):** el usuario reportó `PIECE TEST: 0 <numero grande ~515> 0 0` en vez de `0 0 3 2`.
- **Causa raíz:** `gs.piece.type/rotation/x/y` son campos `u8`/`s8` de un struct. Pasados directamente como argumentos variádicos a `consoleDrawText(..., "%u %u %d %d", ...)`, el toolchain (816-tcc) los empuja como 1 byte crudo cada uno, mientras el lector interno de `%u`/`%d` de `consoleDrawText` consume 2 bytes por argumento (no hay promoción automática a `int` de 16 bits para lecturas directas de campos de struct, a diferencia de un valor de retorno de función, que sí llega en un slot completo — por eso las pruebas de `board_get`/`board_is_cell_occupied` de las Stories 2.1/2.2, que imprimen *resultados de llamada a función*, no mostraban el problema).
  - Prueba matemática: `515 = 0x0203`. `0x03=3` y `0x02=2` son exactamente nuestros valores de `x`/`y` — confirma que el lector de `%u` de "rotation" terminó leyendo los 2 bytes crudos de `x` y `y` como si fueran un solo valor de 16 bits, por el corrimiento de 1 byte que arrastra desde el primer argumento (`type`).
- **Corrección aplicada:** cast explícito de cada campo a un tipo de 16 bits en el sitio de la llamada — `(u16)gs.piece.type`, `(u16)gs.piece.rotation`, `(s16)gs.piece.x`, `(s16)gs.piece.y` — sin tocar `piece_spawn()`, `ActivePiece` ni `GameState` (esos ya eran correctos; el bug era solo en cómo se pasaban a `consoleDrawText`).
- **Riesgo latente no corregido (fuera de alcance de este fix, el usuario pidió investigar solo esta discrepancia):** la prueba de la Story 3.2 (`consoleDrawText(1, 22, "PIECE DATA TEST: %u", piece_shapes[0][1][0])`) imprime un `u8` de array directo, mismo patrón de riesgo — no se tocó porque no fue la discrepancia reportada, pero podría mostrar el mismo tipo de valor incorrecto si se valida visualmente.
- **No verificado por el agente:** resultado visual en emulador tras el fix — no hay emulador en este entorno; pendiente de que el usuario confirme `PIECE TEST: 0 0 3 2`.

### File List

- `snes/source/piece.h` (nuevo) — declaración de `piece_spawn`.
- `snes/source/piece.c` (nuevo) — implementación de `piece_spawn` (type/rotation/x/y fijos).
- `snes/source/main.c` (modificado) — `#include "piece.h"`, prueba de la Story 3.1 reemplazada por `piece_spawn(&gs)` real; luego corregida para castear `type/rotation/x/y` a `u16`/`s16` antes de pasarlos a `consoleDrawText` (fix de bug de varargs). Resto sin cambios.

### Change Log

| Fecha | Cambio |
| --- | --- |
| 2026-07-11 | Implementación de Story 3.3: `piece.c`/`piece.h` con `piece_spawn()` (pieza fija, posición inicial), adaptado de `Game::next()` de Apotris. `make` compila sin errores. Status → in-progress (confirmación visual en emulador pendiente del usuario). |
| 2026-07-11 | Fix: validación visual del usuario mostró `PIECE TEST: 0 ~515 0 0` en vez de `0 0 3 2`. Causa raíz: campos `u8`/`s8` de struct pasados sin promoción a `consoleDrawText` (%u/%d espera slots de 2 bytes, el campo se empuja como 1 byte crudo → corrimiento de bytes). Corregido con cast explícito a `u16`/`s16` en el sitio de la llamada. `make` compila sin errores tras el fix. |
