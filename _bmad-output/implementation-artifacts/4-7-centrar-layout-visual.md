---
baseline_commit: f23bfb42375b3e27f70d1006d583ac8d696e57f6
---

# Story 4.7: Centrar el layout visual (playfield + área de depuración)

Status: review

## Nota de alcance

Story de refinamiento visual puro: asset + un desplazamiento constante de coordenadas en `render.c` y `main.c`. **Cero cambios** a `board.c`, `piece.c`, `input.c`, `game_state.h` — ninguna regla de juego, colisión, gravedad, lock, spawn ni line-clear se toca. El playfield lógico sigue siendo 10×20 (`BOARD_WIDTH`/`BOARD_HEIGHT` sin cambios). `render.c` se toca solo para sumar un offset fijo de filas — sigue de solo-lectura sobre `GameState`, sin nueva regla de juego.

## Story

Como desarrollador del proyecto,
quiero que el conjunto playfield + área de depuración esté centrado en la pantalla y sin tiles sueltos debajo,
para que el layout se vea terminado y no como un placeholder a medio ajustar.

## Contexto (ya establecido, no repetir investigación)

- Pantalla visible: 32×28 tiles (256×224px, `BG_MODE1`, confirmado en la investigación de la Story 4.5 contra el ejemplo oficial `Mode1Png`).
- `playfield.png` (Story 4.5): 256×160px = **32×20 tiles** — cubre filas 0-19 del nametable. Filas 20-27 (8 filas, dentro del área visible de 28) **no están definidas por nuestro `.map`** — quedan con lo que sea que tenga VRAM sin inicializar, que es lo que se ve como "cuadrícula o tiles sueltos debajo del playfield".
- Contenido real (Story 4.4/4.5): relleno + borde de acento en columnas 0-9 (10 tiles), filas 0-19.
- Texto de depuración (Story 4.4): columna inicial 11, filas 2 a 21 (columna 10 como separador).
- Bounding box actual del conjunto: columnas 0-31 (ya cubre el ancho completo — no hace falta centrado horizontal), filas 0-21 (22 de 28 filas) — margen actual: 0 filas arriba, 6 abajo. Para centrar verticalmente: desplazar **todo** el conjunto 3 filas hacia abajo (3 arriba + 3 abajo = 6, igual al margen total).
- `render.c:47-48` ya resta 2 filas de buffer superior del tablero al convertir `piece.y` a píxel de pantalla: `screen_y = (piece.y + row - 2) * 8`. Centrar verticalmente 3 filas más significa sumar 3 a ese cálculo: `-2 + 3 = +1`.

## Acceptance Criteria

1. **Given** `playfield.png` (256×160px, 32×20 tiles), **when** se reemplaza por un canvas de 256×224px (32×28 tiles, cubriendo el alto visible completo), **then** las filas fuera del contenido real quedan en el mismo relleno plano usado en las columnas 10-31 (Story 4.5) — ninguna fila del área visible queda sin definir en el `.map`.
2. **Given** el nuevo canvas, **when** se ubica el contenido real (relleno + borde de acento, 10×20 tiles) dentro de él, **then** ocupa las columnas 0-9 y las **filas 3-22** (antes 0-19) — desplazado 3 filas hacia abajo para centrar el conjunto verticalmente.
3. **Given** `render.c:47-48`, **when** se ajusta el cálculo de posición de los sprites de la pieza activa, **then** `screen_y = (piece.y + row - 2) * 8` pasa a `screen_y = (piece.y + row + 1) * 8` (equivalente a sumar 3 filas, mismo desplazamiento que el asset) — sin cambiar `screen_x`, sin cambiar la lógica de qué celdas ocupa la pieza (`piece.x`/`piece.y`/`piece_shapes` intactos).
4. **Given** las llamadas `consoleDrawText` en `main.c` (columna 11, filas 2 a 21 — Story 4.4), **when** se reubican, **then** cada fila se desplaza +3 (nueva fila = fila original + 3; ej. fila 2→5, fila 21→24), misma columna (11), mismo contenido/formato — solo cambia el argumento de fila.
5. **Given** `board.c`, `piece.c`, `input.c`, `game_state.h`, **when** la Story termina, **then** ninguno muestra diff nuevo atribuible a esta Story (`git diff --stat` no debería agregar líneas ahí).
6. **Given** el proyecto, **when** se ejecuta `make`, **then** compila y linkea sin errores (sin `make clean` — ROM lista para Ares).
7. **Given** la ROM corriendo en Ares, **when** el desarrollador observa la pantalla, **then** el conjunto playfield + área de depuración se ve centrado verticalmente (margen similar arriba y abajo), sin ninguna cuadrícula o tile suelto debajo del playfield, y la pieza activa (sprites) sigue alineada con el fondo del playfield en su nueva posición — confirmado manualmente por el usuario (no verificable por el agente).

## Tasks / Subtasks

- [x] **Task 1: Ensanchar el canvas de `playfield.png` a 32×28 y desplazar el contenido real** (AC: #1, #2)
  - [x] Regenerar `snes/playfield.png` a 256×224px (32×28 tiles): relleno plano en todo el canvas (mismo color que las columnas 10-31 de la Story 4.5), con el bloque real (relleno + borde de acento, 10×20) ubicado en columnas 0-9, filas 3-22.
  - [x] Mantener paleta-indexada (`P` mode), mismas convenciones de las Stories 4.4/4.5.

- [x] **Task 2: Ajustar el offset de los sprites de la pieza activa** (AC: #3)
  - [x] En `render.c`, cambiar `screen_y = (u16)((gs->piece.y + row - 2) * 8);` por `screen_y = (u16)((gs->piece.y + row + 1) * 8);` — mismo mecanismo (constante fija), sin tocar `screen_x` ni ninguna otra línea. Comentario de cabecera de la función actualizado para reflejar el nuevo offset neto (-2+3=+1).

- [x] **Task 3: Desplazar el texto de depuración +3 filas** (AC: #4)
  - [x] En `main.c`, sumado 3 a la fila de cada llamada `consoleDrawText` que usa columna 11 (filas 2, 4, 6, 8, 10, 11, 12, 14, 16, 18, 20, 21 → 5, 7, 9, 11, 13, 14, 15, 17, 19, 21, 23, 24). Mismo contenido/formato, columna sin cambios.

- [x] **Task 4: Build y verificación** (AC: #5, #6)
  - [x] `make` — `playfield.pic`/`.map`/`.pal` regenerados sin error (`gfx4snes` confirma "managed a map of 32x28 tiles"); ROM compila/linkea sin errores.
  - [x] `git diff --stat` — `board.c`, `piece.c`, `input.c`, `game_state.h` sin diff.
  - [x] `make clean` **no** ejecutado — ROM disponible para validación manual en Ares.

## Dev Notes

- **Por qué 3 filas y no otro número:** bounding box actual (filas 0-21, 22 de 28 filas visibles) deja 6 filas de margen, todas abajo. Repartido en partes iguales arriba/abajo = 3 cada lado. Si al validar en Ares el resultado no se ve centrado (ej. por overscan del emulador), es un ajuste de un solo número (el offset), no un rediseño.
- **Por qué tocar `render.c` es aceptable pese a "mantener separados render y lógica":** el cambio es puramente de posicionamiento en pantalla (offset fijo de píxeles), no una regla de juego — `render.c` sigue siendo de solo-lectura sobre `GameState` (`game-architecture.md#2`), no decide nada nuevo. Es el mismo tipo de cambio que ya hizo la Story 4.1 al fijar la resta `-2` original.
- **No repetir el diagnóstico de las Stories 4.5/4.6** — el ancho de 32 tiles y la dependencia de build (`data.obj`) ya están resueltos; esta Story solo agrega alto (20→28 filas) y desplaza contenido, no vuelve a tocar el Makefile ni el stride.
- **Memoria estática, C puro** (`project-context.md`) — el fix es asset + una constante en dos archivos, no requiere estructuras nuevas.
- **Validación:** al finalizar, ejecutar únicamente `make` (no `make clean`) — la ROM debe quedar disponible para Ares. Secuencia completa: 1) `make`, 2) ejecutar en Ares, 3) esperar validación manual del usuario, 4) recién entonces considerar la Story terminada.
- **Anti-patrón a evitar:** no aprovechar para adelantar Stories futuras (HUD real, next-queue, hold) ni para rediseñar el borde/paleta de las Stories 4.4/4.5 — solo centrar y rellenar filas vacías.

### Project Context Rules

- Memoria estática, sin alloc dinámica (`project-context.md#Defaults de implementación`).
- No investigar de nuevo lo ya resuelto (ancho de tilemap, dependencias de build) (`project-context.md#Investigación externa`).
- Mantener el cambio lo más chico posible: un asset + una constante en `render.c` + coordenadas de fila en `main.c`.
- Validación: solo `make` al final (no `make clean`) — ROM lista para Ares.

### References

- `_bmad-output/implementation-artifacts/4-4-layout-playfield-definitivo.md` (arte real, inventario de `consoleDrawText`).
- `_bmad-output/implementation-artifacts/4-5-fix-ancho-tilemap-playfield.md` (canvas 32 tiles de ancho, patrón de relleno plano fuera del área real).
- `_bmad-output/implementation-artifacts/4-6-fix-dependencias-build-dataobj.md` (build ya corregido — los cambios de esta Story SÍ van a llegar a la ROM).
- `snes/source/render.c:47-48` (fórmula de conversión tablero→pantalla, único punto a ajustar).
- `snes/source/main.c` (llamadas `consoleDrawText`, columna 11, filas listadas arriba).

## Dev Agent Record

### Agent Model Used

claude-sonnet-5

### Debug Log References

- `make` (tras regenerar `playfield.png` a 256×224): `gfx4snes` confirma `"process image (256x224px, 256colors)"`, `"managed a map of 32x28 tiles of 8x8 pixels"`, `"saving map file [playfield.map] of (32x28) tiles"` — canvas y mapa correctos.
- Gracias al fix de la Story 4.6 (`data.obj: ...` en el Makefile), este `make` reensambló `data.obj` y re-linkeó `apotris.sfc` automáticamente al cambiar solo `playfield.png` — sin necesitar ningún `touch data.asm` manual (confirmado viendo `wla-65816 ... -o data.obj data.asm` y `wlalink ...` en el log del mismo `make`).
- `git diff --stat` sobre `board.c`/`piece.c`/`input.c`/`game_state.h`: sin salida (cero diff), confirmado.

### Completion Notes List

- **Asset:** `snes/playfield.png` regenerado a 256×224px (32×28 tiles, alto visible completo) — relleno plano en todo el canvas, bloque real (relleno + borde de acento, 10×20) desplazado a columnas 0-9, filas 3-22 (antes filas 0-19). Ya no quedan filas del área visible sin definir en el `.map` — se acabó la "cuadrícula sobrante debajo del playfield" (esa zona ahora es el mismo relleno plano que ya usaban las columnas 10-31 desde la Story 4.5).
- **`render.c`:** `screen_y` de los sprites de la pieza activa pasa de `(piece.y + row - 2) * 8` a `(piece.y + row + 1) * 8` (offset neto -2+3=+1) — mismo mecanismo de constante fija que ya existía, ahora compensa el desplazamiento de 3 filas del asset. Comentario de cabecera actualizado. Sin cambios a `screen_x`, a qué celdas ocupa la pieza, ni a ninguna otra lógica de `render.c`.
- **`main.c`:** las 24 llamadas `consoleDrawText` de columna 11 desplazadas +3 filas (mismo contenido/formato en cada una). El bloque de las Stories 1.1-3.7 (splash + pruebas de boot) y el indicador de tecla de la Story 4.2 quedan en las mismas filas relativas entre sí, solo corridas como conjunto.
- **Sin cambios a `board.c`/`piece.c`/`input.c`/`game_state.h`** — confirmado por `git diff --stat`.
- **AC #7 (confirmación visual en Ares — centrado, sin cuadrícula sobrante, sprites alineados, debug fuera del playfield) pendiente del usuario** — no hay emulador en este entorno.

### File List

- `snes/playfield.png` (modificado) — canvas 256×224 (32×28 tiles), contenido real corrido a filas 3-22, relleno plano en el resto.
- `snes/source/render.c` (modificado) — offset de `screen_y` de sprites ajustado de `-2` a `+1` (compensa el desplazamiento de 3 filas del asset); comentario actualizado.
- `snes/source/main.c` (modificado) — filas de las 24 llamadas `consoleDrawText` desplazadas +3.

## Change Log

| Fecha | Cambio |
| --- | --- |
| 2026-07-12 | Implementación de Story 4.7 (centrar layout visual): `snes/playfield.png` ensanchado a 32×28 tiles (256×224px) con el contenido real corrido a filas 3-22, eliminando las filas sin definir que se veían como cuadrícula sobrante debajo del playfield. `render.c` ajusta el offset de los sprites de la pieza activa (`-2`→`+1`) para seguir alineados con el nuevo fondo. `main.c` desplaza +3 filas todas las llamadas `consoleDrawText`. Sin cambios a `board.c`/`piece.c`/`input.c`/`game_state.h`. Gracias al fix de la Story 4.6, `make` reensambló y re-linkeó todo automáticamente sin workaround manual. `make clean` no ejecutado, ROM disponible para Ares. Status → review (validación visual pendiente del usuario). |
