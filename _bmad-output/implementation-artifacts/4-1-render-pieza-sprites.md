---
baseline_commit: 248e9a8be96dd1a7d6cb9dd97f782db833f834b8
---

# Story 4.1: Render de la pieza activa vía sprites OAM

Status: in-progress (implementación + build verificados; confirmación visual en emulador pendiente del usuario)

## Story

Como jugador,
quiero ver la pieza activa en pantalla mientras cae y se mueve,
para poder jugar visualmente en vez de depender de la consola de texto.

## Alcance (mismo tamaño que las Stories anteriores)

**Sí implementar:**
- Nuevo módulo `render.c`/`render.h` — único módulo con headers de video (`oam.h`), de solo lectura sobre `GameState` (`game-architecture.md#2, #7`).
- `render_init(void)`: inicializa los gráficos de sprite de la pieza vía `oamInitGfxSet` (una sola vez, junto al resto del init de video en `main.c`, antes de `setScreenOn()`).
- `render_sync_piece(GameState *gs, u8 active)`:
  - Si `active == 0`: oculta los 4 sprites (id 0-3) usados para la pieza.
  - Si `active == 1`: recorre `piece_shapes[gs->piece.type][row][col]` (4×4); por cada celda ocupada, posiciona el sprite correspondiente (`oamSet`/`oamSetXY`) en la celda de pantalla correcta y lo muestra.
- Conversión celda lógica → píxel de pantalla: `BoardState.board` tiene 22 filas (20 visibles + 2 de buffer superior para spawn/top-out, `game-architecture.md#4`); un sprite en la celda lógica `(x, y)` va en pantalla `(x*8, (y-2)*8)` — tiles de 8×8, mismo tamaño ya confirmado por el playfield (Story 1.2).
- Integrar en `main.c`: `render_init()` junto al resto del init de video; una llamada a `render_sync_piece(&gs, 1)` después de cada punto ya instrumentado donde la pieza cambia de posición (spawn, movimiento, gravedad — Stories 3.3-3.7) y `render_sync_piece(&gs, 0)` justo después de `piece_lock()`. Reusa la secuencia de pruebas ya existente, sin agregar un loop de juego en vivo.
- Si la Story necesita un asset gráfico de sprite para los bloques de la pieza, adaptarlo desde `reference/apotris` durante la implementación (o un placeholder mínimo si no hay equivalente reusable) — es una decisión de la sesión de `dev-story`, no de esta Story.

**NO implementar todavía:**
- Color por tipo de pieza (tabla `colors[7][3]` de Apotris, ya citada en Story 3.1) — un solo tile/paleta alcanza para "verla en pantalla en la posición correcta".
- Rotación.
- Input real de pad — Story 4.2.
- DMA del tilemap del playfield — Story 4.3.
- Loop de render conectado a gravedad/pad en tiempo real — sigue siendo la secuencia de arranque estática ya existente (mismo patrón desde Story 1.1).
- Cambios a `board.c`/`.h`, `piece.c`/`.h`, `piece_data.c`/`.h`, `game_state.h` — ninguno de estos necesita cambios para esta Story.

## Acceptance Criteria

1. **Given** los gráficos de sprite de la pieza (fuente a resolver durante la implementación), **when** arranca la ROM, **then** `render_init()` los carga vía `oamInitGfxSet` sin error de compilación/link.
2. **Given** una pieza activa spawneada, **when** se llama `render_sync_piece(&gs, 1)`, **then** los 4 sprites OAM correspondientes a las celdas ocupadas de `piece_shapes[type]` quedan visibles en la posición de pantalla correcta (`x*8`, `(y-2)*8`).
3. **Given** la secuencia de pruebas ya existente (spawn, movimiento, gravedad), **when** cada paso cambia la posición de la pieza, **then** se llama `render_sync_piece(&gs, 1)` inmediatamente después, reflejando la nueva posición.
4. **Given** `piece_lock()` ya ejecutado, **when** se llama `render_sync_piece(&gs, 0)` inmediatamente después, **then** los 4 sprites quedan ocultos.
5. **Given** el proyecto compila con `make`, **when** se agregan `render.c/.h` y su integración en `main.c`, **then** no hay errores de compilación/link y las Stories anteriores (1.1-3.7) siguen funcionando.
6. **Given** la ROM corre en un emulador (Ares), **when** se llega al estado tras el boot, **then** se ven visualmente hasta 4 bloques de sprite en la posición esperada sobre el playfield y desaparecen tras el lock — confirmado manualmente por el usuario (no verificable por el agente).

## Tasks / Subtasks

- [x] **Task 1: `render.h`/`render.c` nuevos** (AC: #1, #2, #3, #4)
  - [x] `render_init(void)`: `oamInitGfxSet(...)` una sola vez.
  - [x] `render_sync_piece(GameState *gs, u8 active)`: loop 4×4 sobre `piece_shapes[gs->piece.type][row][col]`, posiciona/oculta hasta 4 sprites según `active`.
  - [x] `render.c` no escribe `GameState` — solo lectura, único módulo con headers de video (`game-architecture.md#7`).

- [x] **Task 2: Resolver asset gráfico de sprite de pieza** (AC: #1)
  - [x] No se encontró equivalente directamente reusable de `reference/apotris` (sprites de GBA, formato/paleta no compatible con PVSnesLib sin trabajo de conversión fuera de alcance) — se creó un placeholder mínimo (`piece.png`, tile sólido de 8×8, paleta de 2 colores).
  - [x] Regla de conversión agregada al `Makefile` (`piece.pic: piece.png`, análoga a `playfield.pic`) e incluida en `bitmaps`.
  - [x] Hallazgo durante la implementación (no anticipado en la Story): GFXCONV por sí solo no genera los símbolos enlazables reales — el proyecto los define a mano en `data.asm` (`playfieldtiles`/`playfieldpal`/etc., ya existente desde Story 1.2). Se agregó un bloque análogo en `data.asm` (`piecetiles`/`piecepal`) envolviendo `piece.pic`/`piece.pal` — sin este paso el link falla (`Unresolved reference to "piecetiles_end"`).

- [x] **Task 3: Integrar en `main.c`** (AC: #2, #3, #4, #5)
  - [x] `render_init()` junto al resto del init de video, antes de `setScreenOn()`.
  - [x] `render_sync_piece(&gs, 1)` después de: `piece_spawn()` (fila 8), la secuencia de movimiento (fila 10), la secuencia de gravedad (fila 12).
  - [x] `render_sync_piece(&gs, 0)` justo después de `piece_lock()` (fila 14).
  - [x] No se tocaron las pruebas de board/colisión (fila 6) ni la de línea (fila 18/20, Story 2.3).

- [x] **Task 4: Build** (AC: #5, #6)
  - [x] `make` compila y linkea sin errores; Stories 1.1-3.7 siguen funcionando.
  - [x] `make clean` **no** ejecutado — ROM (`apotris.sfc`) queda disponible para validación manual en Ares.

## Dev Notes

- **Ownership y separación (`game-architecture.md#2, #7`):** `render.c` es el único módulo que incluye headers de video de PVSnesLib (`background.h`, `oam.h`) y el único que escribe VRAM/OAM; lee `GameState` de solo lectura, nunca lo escribe. `board.c`/`piece.c`/`piece_data.c`/`queue.c` no incluyen headers de video — esta Story no les agrega ninguno.
- **Offset de filas de buffer (crítico):** `BoardState.board` tiene 22 filas — 20 visibles + 2 de buffer superior para spawn/top-out (`game-architecture.md#4`). El playfield visual (Story 1.2) ocupa las 20 filas inferiores, ancladas a pantalla desde el borde superior. Por eso la conversión a píxel debe restar 2 a la fila lógica (`(y-2)*8`) antes de multiplicar por el tamaño de tile — omitir esta resta desalinea la pieza 16px respecto al playfield.
- **Estrategia de sprites (`game-architecture.md#8`):** hasta 4 sprites OAM, uno por celda ocupada de la forma rotación-0 (`piece_shapes[type]`, Story 3.2); reposicionados cada vez que la pieza cambia (barato, no toca el tilemap); ocultos inmediatamente tras el lock, hasta el siguiente spawn.
- **Reutilización del patrón de iteración 4×4:** la Story 3.7 ya resolvió el recorrido de `piece_shapes[type][row][col]` en `piece.c` (`piece_shape_collides`/`piece_lock`) — `render.c` reusa el mismo patrón de iteración (no las funciones de `piece.c`; `render.c` hace su propia lectura, de solo lectura, sin duplicar lógica de colisión/lock).
- **Sin función específica de Apotris que adaptar:** Apotris corre en GBA con su propio hardware de sprites — no hay un algoritmo de "render de sprites" de Apotris directamente portable a PVSnesLib/SNES. La única tabla de datos reusable relacionada (`colors[7][3]`, `tetromino.cpp:9-17`, ya citada en Story 3.1) es de color-por-tipo, explícitamente diferida en esta Story (un solo tile/paleta alcanza para el AC de "verla en la posición correcta").
- **Assets:** el gráfico de sprite del bloque de pieza se resuelve durante la implementación — adaptar algo de `reference/apotris` si aplica, o un placeholder mínimo si no. No es una decisión tomada en esta Story de planificación.
- Memoria estática, sin alloc dinámica, C puro (`project-context.md`). No implementar features fuera de este alcance sin pedido explícito (`project-context.md#Anti-patrones para agentes`).

### Project Context Rules

- Memoria estática, sin allocación dinámica (`project-context.md#Defaults de implementación`).
- C puro, sin C++/STL (`project-context.md#Stack técnico objetivo`).
- Sin ASM 65C816 (no aplica, no hay cuello de botella medido).
- No implementar features fuera del alcance mínimo fijado arriba sin pedido explícito (`project-context.md#Anti-patrones para agentes`).
- **Validación (nueva instrucción del usuario para esta y futuras Stories):** al finalizar la implementación, ejecutar únicamente `make` (no `make clean`) — la ROM debe quedar disponible para cargar en Ares. La secuencia de validación completa sigue siendo: 1) `make`, 2) ejecutar en Ares, 3) esperar validación manual del usuario, 4) recién entonces considerar la Story terminada.

### References

- `_bmad-output/planning-artifacts/epics.md#Epic 4 / Story 4.1`.
- `_bmad-output/game-architecture.md#2 Módulos y responsabilidad, #4 Estructuras de datos principales, #7 Separación gameplay/render, #8 Estrategia mínima de tilemap/VRAM, #9 Inicialización mínima, #10 Orden recomendado de implementación`.
- `_bmad-output/project-context.md#Alcance mínimo, Defaults de implementación, Anti-patrones para agentes`.
- `_bmad-output/implementation-artifacts/3-7-pieza-forma-completa.md` (`piece_shapes`/iteración 4×4 ya establecida).
- `_bmad-output/implementation-artifacts/3-2-datos-piezas.md` (`piece_shapes`, convención de índices).
- `_bmad-output/implementation-artifacts/1-2-playfield-vacio.md` (tiles de 8×8 ya confirmados para el playfield).

## Dev Agent Record

### Agent Model Used

claude-sonnet-5

### Debug Log References

- `make` (1er intento): falló en link — `Unresolved reference to "piecetiles_end"`. `piece_data.as`/`piece.inc` generados por GFXCONV usan `piece_til`/`piece_tilend` (no `piecetiles`/`piecetiles_end`), y esos archivos no son los que realmente se linkean — el proyecto define los símbolos reales a mano en `data.asm` (confirmado comparando con el bloque ya existente de `playfieldtiles`, que tampoco coincide con el `.inc` de GFXCONV). Corregido agregando el bloque `piecetiles`/`piecepal` en `data.asm`.
- `make` (2do intento): compiló y linkeó sin errores. ROM: 90.75% libre. `make clean` **no** ejecutado (instrucción del usuario).

### Completion Notes List

- `render.c`/`render.h` nuevos: `render_init(void)` (llama `oamInitGfxSet` una sola vez) y `render_sync_piece(GameState *gs, u8 active)` (si `active==0` oculta los 4 sprites; si `active==1` recorre `piece_shapes[gs->piece.type]` 4×4 y posiciona/muestra un sprite por celda ocupada). `render.c` es de solo lectura sobre `GameState` — no escribe `gs->piece`/`gs->board`. Único módulo que incluye `<snes.h>` para OAM además de `main.c`.
- Conversión celda lógica → píxel: `(x*8, (y-2)*8)` — el `-2` compensa las 2 filas de buffer superior de `BoardState` (22 filas totales, 20 visibles), ya documentado en la Story.
- `board.c`, `piece.c`, `piece.h`, `piece_data.c`/`.h`, `game_state.h` **sin cambios** — solo lectura desde `render.c`, tal como pedía la Story.
- `main.c`: agregado `#include "render.h"`, `render_init()` junto al resto del init de video (antes de `setMode`/`setScreenOn`), y `render_sync_piece(&gs, 1)` en los 3 puntos ya instrumentados (spawn fila 8, movimiento fila 10, gravedad fila 12) + `render_sync_piece(&gs, 0)` justo después de `piece_lock()` (fila 14). No se tocó la secuencia de pruebas en sí (mismos valores/lógica de las Stories 3.3-3.7 y 2.3).
- Asset: `piece.png` (placeholder, 8×8, 2 colores) creado nuevo — no había gráfico de sprite reusable de `reference/apotris` sin trabajo de conversión (formato GBA) fuera del alcance de esta Story. Regla `piece.pic: piece.png` agregada al `Makefile`.
- Deviación/hallazgo no anticipado por la Story: los símbolos reales enlazados (`piecetiles`/`piecetiles_end`/`piecepal`/`piecepal_end`) no son los que emite `.inc`/`_data.as` de GFXCONV (`piece_til`/`piece_tilend`) — el proyecto ya usaba una capa manual en `data.asm` para esto desde Story 1.2 (`playfieldtiles`, etc.), no documentada explícitamente como paso de esta Story. Se replicó el mismo patrón para `piece`.
- **No verificado por el agente:** resultado visual en emulador (AC #6) — no hay emulador en este entorno; pendiente de confirmación manual del usuario. En particular, confirmar que los sprites del placeholder aparecen sobre el playfield en la posición esperada y desaparecen tras el lock.

### File List

- `snes/source/render.h` (nuevo) — declaraciones de `render_init`/`render_sync_piece`.
- `snes/source/render.c` (nuevo) — implementación; solo lectura sobre `GameState`.
- `snes/source/main.c` (modificado) — `#include "render.h"`, `render_init()` en boot, `render_sync_piece()` en los 4 puntos ya instrumentados (spawn/movimiento/gravedad/lock). Comentario de cabecera actualizado con la entrada de Story 4.1.
- `snes/piece.png` (nuevo) — placeholder de sprite de pieza (8×8, 2 colores).
- `snes/Makefile` (modificado) — regla `piece.pic: piece.png` agregada, incluida en `bitmaps`.
- `snes/data.asm` (modificado) — bloque `piecetiles`/`piecepal` agregado (envuelve `piece.pic`/`piece.pal` con los símbolos que `render.c` referencia), análogo al bloque ya existente de `playfieldtiles`.

### Change Log

| Fecha | Cambio |
| --- | --- |
| 2026-07-11 | Implementación de Story 4.1: `render.c`/`.h` nuevos (sprites OAM de la pieza activa, solo lectura sobre `GameState`), integrados en `main.c` en los puntos ya instrumentados por Stories 3.3-3.7. Asset placeholder `piece.png` creado y wireado (`Makefile` + `data.asm`, siguiendo el patrón ya usado por `playfieldtiles`). `board.c`/`piece.c`/`game_state.h` sin cambios. `make` compila y linkea sin errores (tras corregir el wiring de símbolos de asset); `make clean` no ejecutado, ROM disponible para Ares. Status → in-progress (confirmación visual pendiente del usuario). |
