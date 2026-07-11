---
baseline_commit: 4a5aa4d8514d5c6569661d33eed856ad8ac807dc
---

# Story 3.6: Lock de la pieza activa (punto único)

Status: in-progress (implementación + build verificados; confirmación visual en emulador pendiente del usuario)

## Story

Como desarrollador,
quiero `piece_lock()` que escriba la posición actual de la pieza en el tablero,
para tener el primer "fijado" verificable, antes de nueva pieza, top-out o line clear.

## Alcance (fijado explícitamente por el usuario)

**Sí implementar:**
- Ampliar `snes/source/piece.c`/`piece.h` con `void piece_lock(GameState *gs)`.
- Escribir la posición actual de la pieza (`piece.x`, `piece.y`) en `board[][]`, reutilizando `board_set()` (Story 2.1).
- Seguir con la simplificación de punto único (Stories 3.4/3.5): se fija **una sola celda** (`piece.x`, `piece.y`), no las 4 celdas de la forma completa (`piece_shapes[type][rotation]`) — igual que `piece_move_left/right`/`piece_apply_gravity` usan `board_is_cell_occupied()` sobre un solo punto.
- La pieza permanece en su última posición tras el lock (no se resetea ni se llama `piece_spawn()` — eso es "nueva pieza", explícitamente fuera de esta story).
- Prueba por consola: `piece_spawn()` → aplicar `piece_apply_gravity()` repetidamente hasta que ya no baje más → `piece_lock()` → verificar con `board_get()` que la celda quedó ocupada.

**NO implementar todavía:**
- Line clear.
- Top-out (chequeo de derrota — Apotris lo hace en la misma función `place()`, acá se excluye explícitamente).
- Spawn de nueva pieza tras el lock.
- Render.
- Lock delay (temporizador antes de fijar — acá el lock es inmediato, sin delay).
- Gravedad Q8.8 / velocidad por nivel.
- 7-bag.

## Acceptance Criteria

1. **Given** `gs->piece` en una posición válida (dentro de rango, celda destino libre), **when** se llama `piece_lock(gs)`, **then** `board_get(gs, piece.x, piece.y)` devuelve un valor no-cero (celda ocupada).
2. **Given** una pieza con `type=0` (fija, según Story 3.3), **when** se llama `piece_lock(gs)`, **then** el valor escrito distingue el tipo de pieza (ej. `type + 1`, nunca 0) — consistente con la convención de `board_get`/`board_set` de que `0 = vacío`.
3. **Given** `gs->piece` sin cambios de posición tras el lock, **when** se inspecciona `gs->piece.x`/`gs->piece.y` después de `piece_lock(gs)`, **then** siguen siendo los mismos valores (el lock no mueve ni reinicia la pieza).
4. **Given** el `main.c` existente, **when** arranca la ROM, **then** se ejecuta `piece_spawn()` → gravedad repetida hasta que `piece_apply_gravity()` deje de bajar `piece.y` → `piece_lock()` → se imprime por consola el resultado de `board_get()` en esa celda, confirmando visualmente el lock.
5. **Given** el proyecto compila con `make`, **when** se agrega `piece_lock`, **then** no hay errores de compilación/link y las Stories anteriores (1.1 a 3.5) siguen funcionando sin cambios.

## Tasks / Subtasks

- [x] **Task 1: `piece_lock` en `piece.h`/`piece.c`** (AC: #1, #2, #3)
  - [x] Declarada en `piece.h` junto a `piece_spawn`/`piece_move_left`/`piece_move_right`/`piece_apply_gravity`.
  - [x] `piece_lock(GameState *gs)`: `board_set(gs, (u8)gs->piece.x, (u8)gs->piece.y, gs->piece.type + 1);`.
  - [x] Sin iterar `piece_shapes[type][rotation]`, sin `disappearTimers`/`lowestPart`/`lost`/`lastDrop`/`level`.

- [x] **Task 2: Prueba mínima en `main.c`** (AC: #4, #5)
  - [x] Tras la prueba de gravedad (fila 12, Story 3.5): bucle `do { ... } while (y cambia && iterations < BOARD_HEIGHT)` reutilizando `piece_apply_gravity()` — reutiliza el estado ya asentado por la 3.5 (bloqueado contra la celda de prueba `(3,5)`), sin re-spawn.
  - [x] `piece_lock(&gs)` llamado.
  - [x] Impreso en fila 14: `"LOCK X:%d Y:%d VAL:%u"` (posición final + valor leído con `board_get()`), condensado en una línea.
  - [x] Todos los valores nuevos casteados a `s16`/`u16`.
  - [x] Loop principal, pad, BG0/BG1, `board.c`, `piece_data.c`, `piece_spawn()`, `piece_move_left`/`piece_move_right`, `piece_apply_gravity` y pruebas existentes (filas 2, 4, 6, 8, 10, 12, pad en fila 16) sin cambios.

## Dev Notes

- **Origen de la lógica (Apotris, solo lectura, líneas citadas):** `reference/apotris/source/tetrisEngine.cpp:655-696` (sección de escritura al tablero dentro de `Game::place()`). No se leyó el resto de `place()` (líneas 697+, scoring/T-spin/combo/modos) ni `lockCheck()` (línea 1539, lock-delay, fuera de alcance) — solo el bucle que escribe `board[y][x]`.
  - `board[y][x] = pawn.current + pawn.board[rotation][i][j]` (línea 671) — valor no-cero que codifica id de pieza + bits de forma/borde (puramente visual). Para esta story, simplificado a un único punto y un único valor (`type+1`), sin iterar las 4 celdas de la forma.
  - Chequeo de top-out (líneas 697-709, `lowestPart`/`lost=1`) — explícitamente excluido, tal como pediste.
- **Consistente con la simplificación de punto único ya establecida:** igual que `piece_move_left/right` (Story 3.4) y `piece_apply_gravity` (Story 3.5) usan `board_is_cell_occupied()` sobre `piece.x`/`piece.y` como un solo punto (no las 4 celdas de `piece_shapes[type][rotation]`), `piece_lock()` escribe esa misma única celda. Cuando exista colisión/lock de forma completa (story futura), este punto único se reemplaza sin tocar el resto del código.
- **Prueba de "hasta que ya no pueda bajar":** reutiliza exactamente el mismo mecanismo que ya bloquea la gravedad en la Story 3.5 (la celda de prueba `(3,5)`, ocupada desde el setup de `board_set(&gs, 3, 5, 7)` en `main.c`, o el piso del tablero si se spawnea en una columna distinta) — no hace falta lógica nueva de detección de "tocó fondo", solo repetir `piece_apply_gravity()` y observar cuándo `piece.y` deja de cambiar.
- Sin cambios a `piece_spawn()`, `piece_move_left`/`piece_move_right`, `piece_apply_gravity` (Stories 3.3/3.4/3.5), `board_get`/`board_set`/`board_is_cell_occupied` (Stories 2.1/2.2), `piece_shapes` (Story 3.2).
- Memoria estática, sin alloc dinámica, C puro (`project-context.md`). `piece.c` sigue siendo el único módulo que escribe `gs->piece`; `board.c` sigue siendo el único que escribe `board[][]` directamente — `piece_lock()` pasa por `board_set()`, no accede el array a mano.

### References

- `reference/apotris/source/tetrisEngine.cpp:655-696` (sección de escritura al tablero de `Game::place()`, única parte leída de esa función en esta story)
- `_bmad-output/game-architecture.md#2 Módulos y responsabilidad` (`piece.c`: "invoca `board_*` para colisión/escritura")
- `_bmad-output/project-context.md#Alcance mínimo, Anti-patrones para agentes`
- `_bmad-output/implementation-artifacts/2-1-tablero-logico.md` (`board_set`, ya existente)
- `_bmad-output/implementation-artifacts/3-4-movimiento-horizontal.md`, `3-5-gravedad.md` (mismo patrón de colisión/simplificación de punto único; también documentan el bug de `consoleDrawText` con `u8`/`s8`, aplicar el mismo cast)

## Dev Agent Record

### Agent Model Used

claude-sonnet-5

### Debug Log References

- `make` en `snes/` (PVSNESLIB_HOME=~/Tools/pvsneslib): compiló y linkeó sin errores con `piece_lock` sumada a `piece.c`. `make clean` corrido al final.

### Completion Notes List

- `piece_lock()` escribe un único punto (`piece.x`,`piece.y`) vía `board_set()` — sin iterar la forma completa, sin top-out, sin line clear, sin lock delay, sin re-spawn tras el lock (la pieza queda en su última posición).
- Valor escrito: `type + 1` (nunca 0), distingue el tipo de pieza y respeta la convención `0 = vacío` de `board_get`/`board_set`.
- Prueba en `main.c` reutiliza el estado que dejó la Story 3.5 (ya asentado contra la celda de prueba `(3,5)`) con un bucle `do/while` acotado (`iterations < BOARD_HEIGHT`) en vez de asumir que ya está detenido — cumple literalmente "aplicar gravedad hasta que ya no pueda bajar" aunque en este caso termine en la primera iteración.
- No se tocó `board.c`/`board.h`, `piece_data.c`/`.h`, `game_state.h`, `piece_spawn`/`piece_move_left`/`piece_move_right`/`piece_apply_gravity` ni ninguna Story previa.
- **No verificado por el agente:** resultado visual en emulador (AC #4) — no hay emulador en este entorno; pendiente de confirmación del usuario, igual que en las Stories anteriores.

### File List

- `snes/source/piece.h` (modificado) — declaración de `piece_lock`.
- `snes/source/piece.c` (modificado) — implementación de `piece_lock`; `piece_spawn`/`piece_move_left`/`piece_move_right`/`piece_apply_gravity` sin cambios.
- `snes/source/main.c` (modificado) — prueba mínima en fila 14. Resto sin cambios.

### Change Log

| Fecha | Cambio |
| --- | --- |
| 2026-07-11 | Implementación de Story 3.6: `piece_lock` en `piece.c` (escritura de un único punto vía `board_set`), adaptado de la sección de escritura al tablero de `Game::place()` de Apotris. `make` compila sin errores. Status → in-progress (confirmación visual en emulador pendiente del usuario). |
