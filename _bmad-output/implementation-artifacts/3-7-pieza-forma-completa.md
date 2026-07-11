---
baseline_commit: af9553e28c13889fe53270b45f6bff68b3e3d2e8
---

# Story 3.7: Pieza con forma completa (4 celdas)

Status: in-progress (implementación + build verificados; confirmación visual en emulador pendiente del usuario)

## Nota de desviación respecto a epics.md

Esta Story **no está en `epics.md`**. Se inserta antes de Epic 4 porque las Stories 3.1-3.6 (todas ya cerradas) simplificaron deliberadamente `ActivePiece` a **un único punto** (`piece.x`, `piece.y`), documentado explícitamente en cada una de ellas ("Alcance fijado explícitamente por el usuario"): movimiento, gravedad y lock validan/escriben solo esa celda, ignorando `piece_shapes[type][][]` (ya cargada desde la Story 3.2, pero nunca consultada por colisión/lock).

Epic 4 / Story 4.1 (`epics.md`) asume "hasta 4 sprites, una celda ocupada cada uno" — no se puede renderizar una forma que la lógica todavía no representa. Y no se puede fusionar esta Story con la 4.1 (render): mezclaría lógica (`piece.c`) con render (`render.c`), prohibido explícitamente por las reglas del proyecto para esta ronda de creación de stories.

Decisión del usuario (confirmada): reemplazar ahora la simplificación "pieza = punto" por la representación definitiva de 4 celdas, actualizando las funciones ya existentes de `piece.c`, sin introducir render, rotación, SRS, lock delay ni mecánicas nuevas (top-out, spawn-tras-lock, integración de line-clear). Es lógica pura de Epic 3 (mismo módulo `piece.c`, un único comportamiento: "la pieza colisiona/se fija como sus 4 celdas reales, no como un punto"), validable en una sola ejecución (consola de texto, mismo patrón que 3.1-3.6) — cumple los criterios de fusión/story grande de esta ronda sin violar sus excepciones.

## Story

Como desarrollador,
quiero que `piece.c` valide colisión y escriba el lock usando las 4 celdas reales de `piece_shapes[type]` en vez de un único punto,
para que la representación de la pieza activa sea la definitiva antes de conectar render (Epic 4) o cualquier mecánica nueva.

## Alcance (fijado explícitamente por el usuario)

**Sí implementar:**
- Nueva función interna en `piece.c` (`static u8 piece_shape_collides(GameState *gs, u8 type, s8 x, s8 y)`): recorre las 4×4 celdas de `piece_shapes[type][row][col]`; para cada celda con valor 1, comprueba `board_is_cell_occupied(gs, x+col, y+row)`. Devuelve 1 si cualquier celda ocupada de la forma cae fuera de tablero o sobre una celda ya ocupada.
- `piece_move_left`/`piece_move_right`: reemplazar el chequeo puntual (`board_is_cell_occupied(gs, new_x, gs->piece.y)`) por `piece_shape_collides(gs, gs->piece.type, new_x, gs->piece.y)`.
- `piece_apply_gravity`: mismo reemplazo con `new_y`.
- `piece_lock`: reemplazar la escritura de un único punto por un loop 4×4 sobre `piece_shapes[gs->piece.type][row][col]`; cada celda con valor 1 se escribe con `board_set(gs, (u8)(gs->piece.x+col), (u8)(gs->piece.y+row), gs->piece.type + 1)`.
- `piece_spawn`: sin cambios de comportamiento — sigue fijando `type=0`, posición inicial (`x = BOARD_WIDTH/2-2`, `y = 2`); conceptualmente ya coloca la forma completa (solo cambia qué hacen movimiento/gravedad/lock con esa posición).
- Actualizar las pruebas de `main.c` (filas 10 `MOVE`, 12 `GRAV`, 14 `LOCK`) para que sigan ejecutando el mismo patrón (antes/después impreso por consola) pero ahora sobre colisión/lock de forma completa. No es necesario fijar valores numéricos esperados en las AC — igual que en las Stories 3.4-3.6, el resultado se confirma leyendo la consola.
- `piece.c` debe incluir `piece_data.h` (para `piece_shapes`/`PIECE_GRID_SIZE`), que hoy no incluye.

**NO implementar todavía:**
- Render (sprites/OAM) — Epic 4.
- Rotación real, SRS, kicks — fuera de alcance de todo el proyecto salvo pedido explícito.
- Lock delay (el lock sigue siendo inmediato, igual que en 3.6).
- Top-out / condición de derrota.
- Spawn de nueva pieza tras el lock (la pieza sigue quedando en su última posición, igual que 3.6).
- Integración de line-clear con el lock real (Story 2.3 sigue siendo una prueba aislada en `main.c`, no conectada a `piece_lock`).
- 7-bag / cola de siguiente pieza (`type` sigue fijo en 0).
- Cambios a `board.c`/`board.h`, `piece_data.c`/`.h`, `game_state.h` — ninguno de estos archivos necesita cambios para esta Story.

## Acceptance Criteria

1. **Given** `gs->piece.type` con su forma en `piece_shapes[type]`, **when** se llama `piece_move_left(gs)`/`piece_move_right(gs)`, **then** el movimiento se valida contra las 4 celdas reales de la forma en la posición destino (no solo `piece.x`/`piece.y`) — no se mueve si cualquiera de esas celdas está ocupada o fuera del tablero.
2. **Given** la misma forma, **when** se llama `piece_apply_gravity(gs)`, **then** la caída se valida igual contra las 4 celdas reales de la forma en `y+1` — no desciende si cualquiera colisiona.
3. **Given** una pieza en posición válida, **when** se llama `piece_lock(gs)`, **then** las 4 celdas ocupadas de la forma quedan escritas en `board[][]` (no una sola), cada una con valor `type + 1`.
4. **Given** `piece_spawn(gs)`, **when** se llama, **then** el comportamiento no cambia respecto a la Story 3.3 (mismo `type`, misma posición inicial).
5. **Given** el `main.c` existente, **when** arranca la ROM, **then** las pruebas de movimiento (fila 10), gravedad (fila 12) y lock (fila 14) siguen imprimiendo por consola de texto, ahora reflejando el comportamiento de forma completa.
6. **Given** el proyecto compila con `make`, **when** se agregan estos cambios, **then** no hay errores de compilación/link y las Stories anteriores (1.1-3.6, 2.3) siguen funcionando sin cambios de comportamiento fuera de lo descrito en los AC #1-4.

## Tasks / Subtasks

- [x] **Task 1: `piece_shape_collides` en `piece.c`** (AC: #1, #2, #3)
  - [x] `#include "piece_data.h"` agregado a `piece.c`.
  - [x] `static u8 piece_shape_collides(GameState *gs, u8 type, s8 x, s8 y)`: loop `PIECE_GRID_SIZE`×`PIECE_GRID_SIZE`, `if (piece_shapes[type][row][col] && board_is_cell_occupied(gs, (s8)(x+col), (s8)(y+row))) return 1;`, `return 0;` al final.

- [x] **Task 2: `piece_move_left`/`piece_move_right` sobre forma completa** (AC: #1)
  - [x] Reemplazar `board_is_cell_occupied(gs, new_x, gs->piece.y)` por `piece_shape_collides(gs, gs->piece.type, new_x, gs->piece.y)` en ambas funciones.

- [x] **Task 3: `piece_apply_gravity` sobre forma completa** (AC: #2)
  - [x] Reemplazar `board_is_cell_occupied(gs, gs->piece.x, new_y)` por `piece_shape_collides(gs, gs->piece.type, gs->piece.x, new_y)`.

- [x] **Task 4: `piece_lock` escribe las 4 celdas** (AC: #3)
  - [x] Reemplazar la escritura de un único punto por loop `PIECE_GRID_SIZE`×`PIECE_GRID_SIZE`: `if (piece_shapes[gs->piece.type][row][col]) board_set(gs, (u8)(gs->piece.x+col), (u8)(gs->piece.y+row), gs->piece.type + 1);`.

- [x] **Task 5: Actualizar pruebas en `main.c`** (AC: #4, #5, #6)
  - [x] Fila 10 (`MOVE`): además del par abrir/mover ya existente (izquierda/derecha, sin obstáculo), se agregó un tercer `piece_move_right()` con un obstáculo de prueba en `(7,3)` — la celda más a la derecha de la fila ocupada de la pieza I (`piece.y+1`) si se moviera a `x=4` (columnas 4-7). Esa celda es invisible para el chequeo puntual antiguo (que solo miraba `(new_x, piece.y)`), así que bloquear ese movimiento confirma que la colisión ahora usa `piece_shapes[]` completo, no un punto.
  - [x] Fila 12 (`GRAV`): sin cambios de código (la cadena de 3 pasos ya ejercitaba gravedad); comentario actualizado — desde la Story 3.7 la colisión mira la fila ocupada (`piece.y+1`), un renglón por debajo del punto antiguo, así que ahora frena un paso antes contra la celda de prueba `(3,5)`.
  - [x] Fila 14 (`LOCK`): reemplazado el único `board_get` por un conteo de las 4 celdas de `piece_shapes[gs.piece.type]` en la posición final tras `piece_lock()` — imprime `CNT` (debe ser 4), confirmando que el lock escribe la forma completa.
  - [x] No se tocó la prueba de línea (fila 18/20, Story 2.3) ni la de board/colisión (fila 6) ni `piece_spawn`/`piece_data` (fila 8).
  - [x] `make` compila y linkea sin errores; Stories 1.1-3.6 y 2.3 siguen funcionando (mismas llamadas, ahora con colisión/lock de forma completa). `make clean` ejecutado.

## Dev Notes

- **Origen de la lógica (ya citado en Stories previas, sin releer Apotris):**
  - `board.c` (Story 2.2, comentario existente) ya documenta que `board_is_cell_occupied` es una adaptación de `Game::checkRotation` (`reference/apotris/source/tetrisEngine.cpp:34`) **sin** el loop 4×4 de la forma del pawn — esta Story agrega exactamente ese loop que faltaba, en `piece.c` (no en `board.c`, que no conoce `piece_data.h`/`ActivePiece` por diseño — ver `game-architecture.md#2`).
  - `piece.c` (Story 3.6, comentario existente) ya documenta que `piece_lock` es una adaptación simplificada de la sección de escritura de `Game::place()` (`tetrisEngine.cpp:655-696`), "sin el loop de forma" — esta Story reintroduce ese loop, ya resuelto conceptualmente en esos comentarios previos (no hace falta volver a leer Apotris).
- **Por qué en `piece.c` y no en `board.c`:** `game-architecture.md#2` fija que `board.c` "no conoce pads ni VRAM" pero tampoco piezas — su contrato es `board_is_cell_occupied(gs, x, y)` sobre una celda. `piece.c` es "dueño de la pieza activa... invoca `board_*` para colisión/escritura" — el loop de forma pertenece ahí, reusando `board_is_cell_occupied`/`board_set` célula por célula, sin que `board.c` necesite saber qué es un `ActivePiece`.
- **Convención de índices de `piece_shapes`:** `piece_shapes[type][row][col]`, con `row`→offset en `y`, `col`→offset en `x` (celda de tablero = `piece.x+col`, `piece.y+row`). Coincide con cómo ya se lee en el test de la fila 6 (`piece_shapes[0][1][0]`, Story 3.2) y con el layout visual de las tablas en `piece_data.c` (cada fila del array = fila visual de la pieza).
- **Por qué el test de lock sigue funcionando igual:** la celda de prueba `(3,5)` (seteada en `board_set(&gs, 3, 5, 7)`, fila temprana de `main.c`) cae dentro de las columnas que ocupa la forma I (`type=0`) cuando `piece.x=3` (columnas 3-6) — el mismo obstáculo que ya paraba la gravedad puntual en 3.5/3.6 sigue parando la gravedad de forma completa, sin necesidad de un nuevo setup de tablero.
- Memoria estática, sin alloc dinámica, C puro (`project-context.md`). `piece.c` sigue siendo el único módulo que escribe `gs->piece`; `board.c` sigue siendo el único que escribe `board[][]` directamente (vía `board_set`, ya reusado, no un array nuevo).
- No tocar `board.c`/`board.h`, `piece_data.c`/`.h`, `game_state.h`, `queue.c` (no existe todavía) — esta Story es exclusivamente `piece.c`/`piece.h` (si se necesita declarar algo nuevo, que no es el caso: `piece_shape_collides` es `static`, no se declara en `piece.h`) y las pruebas de `main.c`.

### Project Structure Notes

- Ningún archivo nuevo. Modifica: `snes/source/piece.c` (lógica), `snes/source/main.c` (pruebas). No modifica: `piece.h` (no se agrega ninguna función pública nueva), `board.c/h`, `piece_data.c/h`, `game_state.h`.
- Respeta `game-architecture.md#2`: `piece.c` sigue sin incluir headers de video; `piece_data.h` es una tabla estática neutral de plataforma, su inclusión no rompe la separación gameplay/render.

### Project Context Rules

- Memoria estática, sin allocación dinámica (`project-context.md#Defaults de implementación`).
- C puro, sin C++/STL (`project-context.md#Stack técnico objetivo`).
- Sin ASM 65C816 (no aplica, no hay cuello de botella medido).
- Reutilizar `board_is_cell_occupied`/`board_set` ya existentes en vez de reescribir colisión/escritura (`project-context.md#Defaults de implementación`: preferir reutilización sobre reescritura).
- No implementar features fuera del alcance mínimo fijado arriba sin pedido explícito (`project-context.md#Anti-patrones para agentes`).

### References

- `_bmad-output/game-architecture.md#2 Módulos y responsabilidad` (ownership `piece.c`/`board.c`), `#4 Estructuras de datos principales` (`ActivePiece`, sin campo de forma copiada — referencia a `piece_data`).
- `_bmad-output/planning-artifacts/epics.md#Epic 3` (contexto de por qué esta Story se inserta antes de Epic 4), `#Epic 4 / Story 4.1` (motivo del gap: "hasta 4 sprites... forma correcta").
- `_bmad-output/project-context.md#Defaults de implementación, Anti-patrones para agentes`.
- `_bmad-output/implementation-artifacts/3-2-datos-piezas.md` (`piece_shapes`, convención de índices), `3-4-movimiento-horizontal.md`, `3-5-gravedad.md`, `3-6-lock-pieza.md` (simplificación puntual que esta Story reemplaza; ya citan `tetrisEngine.cpp:34` y `:655-696` de Apotris — no releer).
- `snes/source/piece.c`, `snes/source/board.c`, `snes/source/piece_data.h`, `snes/source/main.c` (estado actual leído completo para esta Story).

## Dev Agent Record

### Agent Model Used

claude-sonnet-5

### Debug Log References

- Sesión 1 — `make` en `snes/` (`PVSNESLIB_HOME=/home/arturo/Tools/pvsneslib`): compiló y linkeó sin errores tras migrar `piece.c` a forma completa. `make clean` corrido al final.
- Sesión 2 — `make`: compiló y linkeó sin errores tras actualizar las pruebas de `main.c` (filas 10/14) para ejercitar/verificar la forma completa. ROM bank 0: 42.00% libre (bajó ~1pp por el conteo de 4 celdas agregado en la prueba de lock). `make clean` corrido al final, sin `git`.

### Completion Notes List

- `piece.c` reemplaza la colisión/lock de punto único (Stories 3.4/3.5/3.6) por la forma completa de `piece_shapes[type]`: nuevo helper `static u8 piece_shape_collides(GameState *gs, u8 type, s8 x, s8 y)` recorre las 4×4 celdas y prueba cada una con `board_is_cell_occupied()` (ya existente, sin cambios). `piece_move_left`/`piece_move_right`/`piece_apply_gravity` ahora llaman a este helper en vez de comprobar un solo punto. `piece_lock` ahora recorre las 4×4 celdas y escribe cada celda ocupada con `board_set(..., type+1)` en vez de una sola.
- API pública sin cambios: mismas firmas de `piece_spawn`/`piece_move_left`/`piece_move_right`/`piece_apply_gravity`/`piece_lock`, mismo contrato ("se mueve si no colisiona, si no no-op"; "lock escribe la posición actual en el tablero"). Solo cambió qué cuenta como "colisiona"/"posición actual" — ahora las 4 celdas reales de la forma, no un punto.
- `main.c` (Task 5, sesión 2): fila 10 agrega un tercer `piece_move_right()` con un obstáculo de prueba en `(7,3)` — celda visible solo para el chequeo de forma completa (invisible para el punto antiguo, que miraba `(new_x, piece.y)`), confirma que la colisión usa `piece_shapes[]`. Fila 12: sin cambio de código, solo comentario — la colisión ahora mira `piece.y+1` (fila ocupada real), un renglón por debajo del punto antiguo, así que la cadena de gravedad frena un paso antes contra la celda de prueba `(3,5)` ya existente. Fila 14: reemplazado el `board_get` de una celda por un conteo de las 4 celdas de la forma en la posición final tras `piece_lock()` (`CNT`, debe dar 4).
- No se tocó `board.c`/`board.h`, `piece_data.c`/`.h`, `game_state.h`, `piece.h` en ninguna sesión.
- **No verificado por el agente:** resultado visual/consola en emulador (no hay emulador en este entorno) — pendiente de confirmación manual del usuario, igual que en las Stories anteriores. En particular, confirmar en consola: `MOVE ... BLK:3>3` (bloqueado), `GRAV:2>3>3>3`, `LOCK ... CNT:4`.

### File List

- `snes/source/piece.c` (modificado) — `piece_shape_collides()` nuevo (static); `piece_move_left`/`piece_move_right`/`piece_apply_gravity`/`piece_lock` reescritos internamente para operar sobre `piece_shapes[type]` completo; `piece_spawn` sin cambios; agregado `#include "piece_data.h"`.
- `snes/source/main.c` (modificado) — fila 10 (`MOVE`): agregado tercer movimiento bloqueado por obstáculo de forma completa. Fila 12 (`GRAV`): solo comentario actualizado. Fila 14 (`LOCK`): reemplazado `board_get` único por conteo de 4 celdas. Comentario de cabecera actualizado con la entrada de la Story 3.7.

### Change Log

| Fecha | Cambio |
| --- | --- |
| 2026-07-11 | Implementación de Story 3.7 (2 sesiones): (1) `piece.c` migrado de colisión/lock de punto único a forma completa (`piece_shapes[type]`) vía nuevo helper `piece_shape_collides`; (2) pruebas de `main.c` (filas 10/14) actualizadas para ejercitar/verificar colisión y lock de forma completa. `make` compila y linkea sin errores en ambas sesiones, `make clean` ejecutado. Status → in-progress (confirmación visual en emulador pendiente del usuario). |
