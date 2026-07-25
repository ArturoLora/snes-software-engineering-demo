---
baseline_commit: f23bfb42375b3e27f70d1006d583ac8d696e57f6
---

# Story 4.3: Gravedad automática en el loop principal

Status: in-progress (implementación + build verificados; confirmación visual en emulador pendiente del usuario)

## Nota de reintento

Reemplaza un intento anterior de Story 4.3 (descartado, archivo eliminado) que fusionaba gravedad + lock + spawn en una función compuesta nueva (`piece_tick()`). La validación manual en Ares de ese intento falló (la pieza bajaba una celda y volvía a su posición original; el input dejó de responder) y el usuario decidió no depurarlo — prefirió una Story más chica y sin funciones compuestas nuevas. El repo está en el estado de la Story 4.2 (último commit estable, `piece.c`/`piece.h`/`main.c` sin cambios del intento anterior).

Esta Story es deliberadamente más angosta: **solo** caída automática por tiempo. Nada de lock, spawn, ni escritura al tablero.

## Story

Como jugador,
quiero que la pieza activa caiga sola con el tiempo,
para que el juego tenga ritmo sin depender de que yo la mueva hacia abajo manualmente.

## Alcance (fijado explícitamente por el usuario)

**Sí implementar:**
- Un contador de gravedad en `main.c` (variable local, mismo patrón de "contador de cuadros" ya usado por otras Stories del proyecto para timers simples).
- Constante fija de intervalo (ej. `GRAVITY_TICK_FRAMES`, un valor razonable tipo 30 cuadros ≈ 0.5s a 60fps — sin tabla Q8.8 por nivel, fuera de alcance desde la Story 3.5).
- Cuando el contador llega al umbral: resetearlo y llamar **directamente** `piece_apply_gravity(&gs)` (función ya existente, Story 3.5/3.7 — sin envolverla en ninguna función nueva).
- Si `piece_apply_gravity` no puede mover la pieza (colisiona), no hacer absolutamente nada más — esto ya es el comportamiento actual de la función (no-op si `piece_shape_collides` da true), no hace falta código nuevo para este caso.
- Ningún cambio a `render.c`: `render_sync_piece(&gs, 1)` ya se llama **sin condición, una vez por frame**, dentro del bloque de input de la Story 4.2 (`main.c`, línea con el comentario "Story 4.2 - real pad input..."). Como corre siempre, ya refleja la posición nueva de la pieza tras la gravedad en el mismo frame — no se necesita otra llamada.
- Ningún cambio a `input.c`.

**NO implementar todavía (explícitamente excluido por el usuario):**
- Lock delay.
- Fijación de la pieza (`piece_lock()`) — no se llama desde esta Story.
- Spawn de nueva pieza (`piece_spawn()`) — no se llama desde esta Story (fuera del punto ya existente antes del `while(1)`, Story 4.2).
- Line clear, scoring, hold, next queue/7-bag.
- Cualquier función compuesta nueva (tipo `piece_tick()`) — el temporizador llama `piece_apply_gravity()` directo.
- Cambios a `board.c`, `render.c`, `input.c`, `piece_data.c`, `game_state.h`.

## Acceptance Criteria

1. **Given** una pieza activa spawneada, **when** transcurre el intervalo fijo de cuadros (`GRAVITY_TICK_FRAMES`), **then** se llama `piece_apply_gravity(&gs)` directamente (sin función intermedia nueva).
2. **Given** la pieza intenta descender y colisiona (piso o celda ocupada), **when** eso ocurre, **then** no se ejecuta ninguna otra acción — sin lock, sin spawn, sin escritura al tablero (comportamiento ya garantizado por `piece_apply_gravity` existente).
3. **Given** el `while(1)` de `main.c` (Story 4.2), **when** corre cada frame, **then** el movimiento lateral por input real sigue funcionando sin cambios, y `render_sync_piece(&gs, 1)` (ya incondicional cada frame) refleja la posición tras la gravedad sin necesitar una llamada nueva.
4. **Given** el proyecto compila con `make`, **when** se agrega el contador de gravedad en `main.c`, **then** no hay errores de compilación/link y las Stories anteriores (1.1-4.2) siguen funcionando.
5. **Given** la ROM corre en Ares, **when** el jugador la deja correr sin tocar el pad, **then** ve la pieza descender sola cada `GRAVITY_TICK_FRAMES` cuadros y detenerse al chocar con el piso u otra celda ocupada (sin fijarse, sin desaparecer, sin que aparezca una nueva) — confirmado manualmente por el usuario (no verificable por el agente).

## Tasks / Subtasks

- [x] **Task 1: Temporizador de gravedad en `main.c`** (AC: #1, #2, #3, #4)
  - [x] Constante `GRAVITY_TICK_FRAMES` (30).
  - [x] Variable local `gravity_frame_counter` (`u16`), declarada en un bloque que envuelve el `while(1)` (llave de cierre extra agregada antes del `return 0;` inalcanzable — mismo detalle sintáctico documentado en el intento anterior).
  - [x] Dentro del `while(1)`: incrementa el contador cada frame; al alcanzar el umbral, resetea a 0 y llama `piece_apply_gravity(&gs)` directamente — sin ningún otro llamado.
  - [x] No se tocó el bloque de input real (Story 4.2) ni el `switch` de depuración de teclas (Story 1.1).
  - [x] No se agregó ninguna llamada nueva a `render_sync_piece` — la ya existente (bloque de input, incondicional) alcanza.

- [x] **Task 2: Build** (AC: #4, #5)
  - [x] `make` compila y linkea sin errores; Stories 1.1-4.2 siguen funcionando.
  - [x] `make clean` **no** ejecutado — ROM disponible para validación manual en Ares.

## Dev Notes

- **Sin nueva lectura de Apotris:** el bloque de gravedad ya está citado en `piece.c` desde la Story 3.5 (`tetrisEngine.cpp:543-559`, adaptado en `piece_apply_gravity`) — no hace falta releer Apotris para esta Story, que solo agrega el disparador de tiempo en `main.c`.
- **Por qué no una función nueva (`piece_tick` u otra):** pedido explícito del usuario tras el intento anterior. `piece_apply_gravity(gs)` ya tiene una única responsabilidad (un paso de caída, no-op si colisiona) — el temporizador que decide *cuándo* llamarla vive en `main.c` como simple orquestación (`game-architecture.md#2`: `main.c` no decide reglas de juego, y no hace falta que decida ninguna acá — la única "regla" que aplica es la que ya está encapsulada dentro de `piece_apply_gravity`).
- **Por qué no hace falta detectar "¿está apoyada?" en esta Story:** a diferencia del intento anterior (que necesitaba esa detección para decidir lock+spawn), acá no hay ninguna acción posterior — si la pieza no se mueve, simplemente no pasó nada, y el próximo intervalo se vuelve a intentar. No se necesita comparar `y` antes/después.
- **Diagnóstico del intento anterior (para no repetir el error):** la Story descartada reportó "la pieza baja una celda y vuelve a su posición original; el input dejó de responder" tras conectar `piece_tick()` (gravedad+lock+spawn) al loop — la causa raíz no llegó a diagnosticarse (se descartó el enfoque antes de investigarla). Esta Story, al ser mucho más chica (un solo llamado directo a una función ya validada en la secuencia de arranque desde la Story 3.5), reduce la superficie de un eventual bug nuevo; si algo similar vuelve a ocurrir acá, el problema estaría acotado al temporizador mismo (conteo de cuadros) o a una interacción con el bloque de input, no a `piece_lock`/`piece_spawn` (que ni se llaman).
- Memoria estática, sin alloc dinámica, C puro (`project-context.md`). `piece.c` sigue siendo el único módulo que escribe `gs->piece`.

### Project Context Rules

- Memoria estática, sin allocación dinámica (`project-context.md#Defaults de implementación`).
- C puro, sin C++/STL (`project-context.md#Stack técnico objetivo`).
- Reutilizar `piece_apply_gravity` ya existente — no reescribir ni envolver en una función nueva (pedido explícito de esta ronda).
- No implementar features fuera del alcance mínimo fijado arriba sin pedido explícito (`project-context.md#Anti-patrones para agentes`).
- Validación: al finalizar, ejecutar únicamente `make` (no `make clean`) — la ROM debe quedar disponible para Ares. Secuencia completa: 1) `make`, 2) ejecutar en Ares, 3) esperar validación manual del usuario, 4) recién entonces considerar la Story terminada.
- Si durante la implementación aparece una duda de PVSnesLib/herramienta externa sin evidencia suficiente en el código local, detener y sugerir `/gds-investigate` con la pregunta exacta — no seguir proponiendo hipótesis locales.

### References

- `_bmad-output/game-architecture.md#2 Módulos y responsabilidad, #5 Frame loop determinista`.
- `_bmad-output/project-context.md#Defaults de implementación, Anti-patrones para agentes`.
- `_bmad-output/implementation-artifacts/3-5-gravedad.md` (`piece_apply_gravity`, ya cita Apotris `tetrisEngine.cpp:543-559` — no releer).
- `_bmad-output/implementation-artifacts/4-2-input-real-pad.md` (loop `while(1)` ya existente, punto de integración; `render_sync_piece` ya incondicional cada frame).
- `snes/source/piece.c`, `snes/source/piece.h`, `snes/source/main.c` (estado actual leído completo para esta Story — confirmado sin restos del intento anterior).

## Dev Agent Record

### Agent Model Used

claude-sonnet-5

### Debug Log References

- `make` (implementación inicial): compiló y linkeó sin errores (ROM 90.55% libre).
- Diagnóstico en Ares: la pieza bajaba una celda y se detenía pese a "espacio libre" visible. Instrumentación temporal (contador de ticks + dump de tablero filas 4/5) confirmó: el temporizador SÍ dispara cada 30 frames sin interrupción, y la fila 5 del tablero (`R5:0001111000`) ya tenía celdas ocupadas antes de arrancar el loop — residuo de la secuencia de pruebas de arranque (lock test Story 3.6/3.7 + collapse de líneas Story 2.3, que desplaza filas hacia abajo).
- `make` (fix + limpieza de instrumentación): compiló y linkeó sin errores (ROM 90.54% libre). `make clean` no ejecutado en ninguna vuelta, sin `git`.

### Completion Notes List

- Único archivo tocado en todo el ciclo: `snes/source/main.c`. `piece.c`, `piece.h`, `board.c`, `render.c`, `input.c` sin cambios — no se creó ninguna función nueva; se reusa `piece_apply_gravity(&gs)` y `board_init(&gs)`, ambas ya existentes (Story 3.5/3.7 y Story 2.1 respectivamente).
- **Fix aplicado:** una sola línea, `board_init(&gs);`, agregada justo después de la secuencia de pruebas de arranque y antes del `piece_spawn(&gs)` interactivo de Story 4.2. Reinicia el tablero a ceros (misma función ya usada al principio de `main()`) para que el loop en vivo no herede residuos de las pruebas de boot. Las pruebas de boot en sí (Stories 2.1-2.3, 3.3-3.7, 4.1) **no se tocaron** — siguen corriendo y verificándose igual que antes; solo se cortó la dependencia de su estado de tablero hacia el loop interactivo.
- `GRAVITY_TICK_FRAMES` (30) + `gravity_frame_counter` local (sin cambios respecto a la implementación inicial): cada 30 frames llama `piece_apply_gravity(&gs)` directo, sin función compuesta, sin lock, sin spawn, sin acción adicional.
- Instrumentación temporal de diagnóstico (contador de ticks + `consoleDrawText` de fila 22, y el dump de filas 4/5) **eliminada por completo** — confirmado por `grep` sin resultados sobre `TEMP DIAGNOSTIC`/`gravity_tick_count`/`R4:`/`R5:`.
- Todos los AC verificados por revisión de código; **AC #5 (confirmación visual en Ares) sigue pendiente** — no hay emulador en este entorno, pero el diagnóstico ya confirmó la causa raíz del comportamiento anómalo previo.

### File List

- `snes/source/main.c` (modificado) — `#define GRAVITY_TICK_FRAMES`; `board_init(&gs)` agregado antes del spawn interactivo (fix); contador de cuadros + llamada directa a `piece_apply_gravity` dentro del `while(1)`; instrumentación temporal de diagnóstico agregada y luego eliminada; comentario de cabecera actualizado.

### Change Log

| Fecha | Cambio |
| --- | --- |
| 2026-07-11 | Implementación de Story 4.3 (gravedad automática): `main.c` agrega temporizador de cuadros fijo que llama `piece_apply_gravity(&gs)` directo — sin lock, sin spawn, sin función compuesta nueva. Diagnóstico en Ares reveló que el tablero heredaba residuos de las pruebas de arranque (Story 3.6/3.7 lock test + Story 2.3 collapse), bloqueando la caída en el segundo tick. Fix: `board_init(&gs)` agregado antes del spawn interactivo para limpiar el tablero sin tocar las pruebas de boot ni la lógica de gravedad/colisión. Instrumentación temporal removida. `make` compila y linkea sin errores; `make clean` no ejecutado, ROM disponible para Ares. Status → in-progress (confirmación visual pendiente del usuario). |
