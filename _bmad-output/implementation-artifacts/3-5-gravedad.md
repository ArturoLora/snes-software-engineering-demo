---
baseline_commit: 757578adc56cbc23a263c1535b97c3f495f1aa5a
---

# Story 3.5: Gravedad (paso fijo, sin temporizador)

Status: in-progress (implementación + build verificados; confirmación visual en emulador pendiente del usuario)

## Story

Como desarrollador,
quiero `piece_apply_gravity()` que baje `piece.y` en 1 si la celda de abajo no está ocupada,
para tener el descenso básico validado por colisión, antes de un temporizador real o Q8.8.

## Alcance (fijado explícitamente por el usuario)

**Sí implementar:**
- Ampliar `snes/source/piece.c`/`piece.h` con `void piece_apply_gravity(GameState *gs)`.
- Mover únicamente `gs->piece.y` (+1) — nada de `x`, `rotation` ni `type`.
- Usar `board_is_cell_occupied()` (Story 2.2) para impedir bajar del tablero — misma simplificación de colisión puntual que la Story 3.4 (un solo punto `piece.x`/`piece.y`, no las 4 celdas de la forma).
- **Sin temporizador real todavía:** cada llamada a `piece_apply_gravity()` equivale a exactamente un paso de gravedad (no hay acumulador `speedCounter`/Q8.8 — eso es explícitamente fuera de esta story).
- Prueba por consola: `piece_spawn()`, luego aplicar gravedad varias veces, imprimiendo `y` antes y después de cada paso.

**NO implementar todavía:**
- Lock (fijar la pieza al tablero cuando no puede seguir bajando).
- Top-out (condición de derrota).
- Line clear.
- Render.
- Gravedad Q8.8 / acumulador de sub-frame (`speedCounter`).
- Velocidad por nivel (tabla `GameInfo::gravity[19]`/curva por nivel).
- Lectura de pad real.

## Acceptance Criteria

1. **Given** `gs->piece` tras `piece_spawn()` (`y=2`) y la celda `(x, y+1)` libre, **when** se llama `piece_apply_gravity(gs)`, **then** `gs->piece.y` incrementa en 1.
2. **Given** `gs->piece.y = BOARD_HEIGHT-1` (piso), **when** se llama `piece_apply_gravity(gs)`, **then** `gs->piece.y` no cambia (bloqueado por `board_is_cell_occupied` al detectar `y+1 >= BOARD_HEIGHT`).
3. **Given** una celda `(x, y+1)` ocupada (fijada por debug con `board_set`), **when** se llama `piece_apply_gravity(gs)`, **then** `gs->piece.y` no cambia.
4. **Given** el `main.c` existente, **when** arranca la ROM, **then** se llama `piece_spawn(&gs)` seguido de varias llamadas a `piece_apply_gravity(&gs)`, imprimiendo `y` antes y después de cada paso, confirmando visualmente el descenso.
5. **Given** el proyecto compila con `make`, **when** se agrega `piece_apply_gravity`, **then** no hay errores de compilación/link y las Stories anteriores (1.1 a 3.4) siguen funcionando sin cambios.

## Tasks / Subtasks

- [x] **Task 1: `piece_apply_gravity` en `piece.h`/`piece.c`** (AC: #1, #2, #3)
  - [x] Declarada en `piece.h` junto a `piece_spawn`/`piece_move_left`/`piece_move_right`.
  - [x] `piece_apply_gravity(GameState *gs)`: `s8 new_y = gs->piece.y + 1; if (!board_is_cell_occupied(gs, gs->piece.x, new_y)) gs->piece.y = new_y;` — un solo paso por llamada.
  - [x] Sin `speedCounter`/`speed`/`gravity[level]`/`gracePeriod`/`zoneTimer`/`rotationSystem==NRS`+`place()`.

- [x] **Task 2: Prueba mínima en `main.c`** (AC: #4, #5)
  - [x] Tras la prueba de movimiento horizontal (filas 24/26): `piece_spawn(&gs)` de nuevo (reinicia `y=2`) + 3 llamadas a `piece_apply_gravity(&gs)` en secuencia, imprimiendo `y` antes/después de cada una (filas 28, 30, 32), mismo patrón de variable local que la Story 3.4.
  - [x] Todos los valores nuevos casteados a `s16`.
  - [x] Loop principal, pad, BG0/BG1, `board.c`, `piece_data.c`, `piece_spawn()` original, `piece_move_left`/`piece_move_right` y pruebas anteriores (filas 12, 16, 20, 22, 24, 26) sin cambios.

## Dev Notes

- **Origen de la lógica (Apotris, solo lectura, líneas citadas):** `reference/apotris/source/tetrisEngine.cpp:543-559`, dentro de `Game::update()`. No se leyó el resto de `update()` (líneas 430-542, 560+) ni ninguna otra función — solo el bloque de gravedad.
  - Acumulador: `speedCounter += speed` (línea 546) — `speed` viene de `GameInfo::gravity[level]` (curva por nivel, `float`), fuera de alcance (esta story no tiene niveles ni Q8.8).
  - `n = (int)speedCounter` (línea 548) — parte entera = pasos a aplicar este frame. Bucle `for i<n` (líneas 549-557): `if (checkRotation(0,1,rotation)) pawn.y++;` (líneas 550-551, núcleo adaptado); rama `else if (rotationSystem==NRS) place();` (línea 554-556) es lock inmediato de otro sistema de rotación — fuera de alcance.
  - `speedCounter -= n` (línea 559) — guarda el resto fraccionario para el próximo frame; no aplica porque esta story no tiene acumulador (una llamada = un paso fijo, según pediste).
  - `gracePeriod`/`zoneTimer` (líneas 543-546) son de otros modos/mecánicas — fuera de alcance.
- **Reutiliza el mismo patrón que `piece_move_left`/`piece_move_right` (Story 3.4):** calcular coordenada candidata → `board_is_cell_occupied()` → mover o no. Mismo tipo de simplificación (colisión puntual, no de forma completa) documentada en esa story.
- Sin cambios a `piece_spawn()`, `piece_move_left`/`piece_move_right` (Story 3.4), `board_is_cell_occupied`/`board_get`/`board_set` (Stories 2.1/2.2), `piece_shapes` (Story 3.2).
- Memoria estática, sin alloc dinámica, C puro (`project-context.md`). `piece.c` sigue siendo el único módulo que escribe `gs->piece`.

### References

- `reference/apotris/source/tetrisEngine.cpp:543-559` (bloque de gravedad dentro de `Game::update()`, única sección leída de ese archivo en esta story)
- `_bmad-output/game-architecture.md#2 Módulos y responsabilidad`, `#5 Frame loop determinista` (menciona el acumulador Q8.8 para stories futuras — no aplica todavía)
- `_bmad-output/project-context.md#Alcance mínimo, Anti-patrones para agentes`
- `_bmad-output/implementation-artifacts/2-2-colision-basica.md` (`board_is_cell_occupied`, ya existente)
- `_bmad-output/implementation-artifacts/3-4-movimiento-horizontal.md` (`piece_move_left`/`piece_move_right`, mismo patrón de colisión puntual; también documenta el bug de `consoleDrawText` con `u8`/`s8`, aplicar el mismo cast)

## Dev Agent Record

### Agent Model Used

claude-sonnet-5

### Debug Log References

- `make` en `snes/` (PVSNESLIB_HOME=~/Tools/pvsneslib): compiló y linkeó sin errores con `piece_apply_gravity` sumada a `piece.c`. `make clean` corrido al final.

### Completion Notes List

- Un paso fijo por llamada (sin acumulador `speedCounter`), colisión puntual (no de forma completa) — igual criterio que `piece_move_left`/`piece_move_right` (Story 3.4).
- `piece_spawn(&gs)` se vuelve a llamar en `main.c` antes de la prueba de gravedad porque la prueba de movimiento (Story 3.4) ya había modificado `x` (no `y`, pero por claridad se reinicia todo el estado antes de esta prueba nueva) — no se tocó `piece_spawn()` en sí.
- No se tocó `board.c`/`board.h`, `piece_data.c`/`.h`, `game_state.h`, `piece_move_left`/`piece_move_right` ni ninguna Story previa.
- **No verificado por el agente:** resultado visual en emulador (AC #4) — no hay emulador en este entorno; pendiente de confirmación del usuario, igual que en las Stories anteriores.

### File List

- `snes/source/piece.h` (modificado) — declaración de `piece_apply_gravity`.
- `snes/source/piece.c` (modificado) — implementación de `piece_apply_gravity`; `piece_spawn`/`piece_move_left`/`piece_move_right` sin cambios.
- `snes/source/main.c` (modificado) — prueba mínima en filas 28/30/32. Resto sin cambios.

### Change Log

| Fecha | Cambio |
| --- | --- |
| 2026-07-11 | Implementación de Story 3.5: `piece_apply_gravity` en `piece.c` (paso fijo, colisión puntual vía `board_is_cell_occupied`), adaptado del bloque de gravedad de `Game::update()` de Apotris. `make` compila sin errores. Status → in-progress (confirmación visual en emulador pendiente del usuario). |
