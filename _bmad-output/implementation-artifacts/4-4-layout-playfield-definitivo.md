---
baseline_commit: f23bfb42375b3e27f70d1006d583ac8d696e57f6
---

# Story 4.4: Layout definitivo del playfield

Status: review

## Nota de alcance

Esta Story es puramente visual/cosmética: arte del playfield + posición del texto de depuración. **Cero cambios** a `board.c`, `piece.c`, `input.c`, `game_state.h` — ninguna función de esos módulos se agrega, quita ni modifica, y ninguna de sus llamadas existentes en `main.c` cambia de lugar u orden. Apotris se usa **solo como referencia visual/de distribución** (paleta, idea de marco alrededor del pozo) — no se lee ni adapta ningún algoritmo de `reference/apotris/source/`.

## Story

Como desarrollador del proyecto,
quiero un playfield con arte definitivo y el texto de depuración reubicado fuera del área de juego,
para poder seguir desarrollando y viendo las próximas Stories sin que el placeholder temporal ni los prints de depuración se superpongan a la zona jugable.

## Acceptance Criteria

1. **Given** `snes/playfield.png` (placeholder actual: 80×160px, grid plano de cuadros grises sobre azul, confirmado por inspección — 10×20 tiles de 8px), **when** se reemplaza por un arte definitivo del mismo tamaño exacto (80×160px, 10×20 tiles) inspirado visualmente en Apotris (ej. relleno sólido + marco/borde de acento, sin el patrón de rejilla plano actual), **then** `make` reconvierte el asset (target `playfield.pic` del Makefile) sin tocar las llamadas `bgInitTileSet`/`bgInitMapSet` en `main.c` (mismos punteros, mismo `SC_32x32`, mismo `BG_16COLORS`).
2. **Given** el nuevo arte, **when** se compara con el placeholder anterior, **then** ya no queda ningún resto del patrón de rejilla temporal (grid de cuadros) en el playfield.
3. **Given** las llamadas `consoleDrawText` existentes en `main.c` (todas las de columnas 1/3/6, filas 2–22 — listadas en Dev Notes), **when** se reubican, **then** ninguna imprime en columnas 0–9 (ancho del playfield); todas empiezan en columna ≥ 11 (columna 10 queda como separador), misma fila que antes, mismo contenido/formato — solo cambia el argumento de columna.
4. **Given** cualquier string relocalizado cuyo ancho exceda las columnas disponibles desde su nueva columna (32 − columna_inicio), **when** se reubica, **then** se acorta o se parte en dos filas para que no se corte ni desborde.
5. **Given** el bloque de instrumentación temporal agregado durante la investigación de la Story 4.3 (comentarios `TEMP DIAGNOSTIC`, variable `debug_collision_reported`, el escaneo `FC ROW/COL/RSN` y su `consoleDrawText` en la fila 22 de `main.c`), **when** se implementa esta Story, **then** ese bloque se elimina por completo (ya cumplió su propósito de diagnóstico, confirmado: colisión contra límite inferior del tablero).
6. **Given** `board.c`, `piece.c`, `input.c`, `game_state.h`, **when** la Story termina, **then** ninguno muestra diff (`git diff --stat` los excluye).
7. **Given** el proyecto, **when** se ejecuta `make`, **then** compila y linkea sin errores (sin `make clean` — ROM lista para Ares).
8. **Given** la ROM corriendo en Ares, **when** el desarrollador observa el boot y el loop interactivo, **then** el área del playfield (columnas 0–9) solo muestra el arte definitivo, sin texto superpuesto, y todo el texto de depuración es legible en la columna lateral — confirmado manualmente por el usuario (no verificable por el agente).

## Tasks / Subtasks

- [x] **Task 1: Arte definitivo del playfield** (AC: #1, #2)
  - [x] Revisar `reference/apotris/sprites/` (ej. `05frameTexture.png`, `19frameTexture2.png`, `08barTexture.png`) solo como inspiración visual de marco/distribución — no copiar píxeles ni adaptar código.
  - [x] Regenerar `snes/playfield.png` con el mismo tamaño exacto (80×160px) y paleta compatible con `BG_16COLORS`, reemplazando el grid plano por un look de pozo con marco/borde.
  - [x] `make` (target `playfield.pic` se reconvierte automáticamente) — si el nuevo arte no entra en la cantidad de colores/tiles ya asumida por `bgInitTileSet`/`bgInitMapSet`, detener y reportar antes de tocar esas llamadas en `main.c`.

- [x] **Task 2: Eliminar instrumentación temporal de la Story 4.3** (AC: #5, #6)
  - [x] Quitar de `main.c` el bloque `TEMP DIAGNOSTIC` (variable `debug_collision_reported`, el escaneo de colisión y su `consoleDrawText` de fila 22) agregado durante la investigación de la Story 4.3.
  - [x] Confirmar por `grep` que no queda ningún rastro (`TEMP DIAGNOSTIC`, `debug_collision_reported`, `debug_prev_y`, `debug_target_y`, `FC ROW`).

- [x] **Task 3: Reubicar texto de depuración** (AC: #3, #4, #8)
  - [x] Catalogar todas las llamadas `consoleDrawText` de `main.c` (columnas 1/3/6, filas 2 a 22 — incluye splash inicial, pruebas de boot de Stories 1.1–3.7, e indicador de tecla presionada de Story 4.2).
  - [x] Mover cada una a columna ≥ 11, misma fila, mismo contenido — sin tocar qué las dispara ni su lógica.
  - [x] Verificar ancho disponible (32 − columna_inicio) contra cada string formateado; acortar o partir en dos filas las que no entren.

- [x] **Task 4: Build** (AC: #7)
  - [x] `make` compila y linkea sin errores.
  - [x] `make clean` **no** ejecutado — ROM disponible para validación manual en Ares.
  - [x] `git diff --stat` confirma que `board.c`, `piece.c`, `input.c`, `game_state.h` no aparecen en el diff.

## Dev Notes

- **Por qué esta Story existe:** investigación previa (Story 4.3, sesión de diagnóstico) confirmó que la pieza se detiene correctamente contra el límite inferior del tablero (fila lógica 22, `board_is_cell_occupied` = límite, no celda ocupada) — la gravedad/colisión funcionan bien. La percepción de "se detiene a la mitad de la pantalla" viene de que `playfield.png` (80×160px = 10×20 tiles) solo cubre 160 de los 224px de alto de pantalla SNES (`BG_MODE1`, sin scroll/overscan tocado) — el playfield fue diseñado a propósito para cubrir solo el 20-filas-visible del tablero (`BOARD_HEIGHT=22`, 2 filas de buffer superior + 20 visibles), no la pantalla completa. **Esta Story NO cambia esa proporción** (no se agranda el playfield a 224px) — solo reemplaza el arte placeholder y reubica el texto. Si en el futuro se decide que el playfield debe ocupar más alto de pantalla, es una Story aparte.
- **Conversión tablero→pantalla ya existente (no tocar):** `render.c:47-48`, `screen_y = (piece.y + row - 2) * 8` — la resta de 2 descuenta el buffer superior del tablero. Esta Story no toca `render.c` en absoluto (ni sprites de la pieza activa ni la fórmula de conversión).
- **Instrumentación pendiente de esta sesión:** el bloque `TEMP DIAGNOSTIC` (Story 4.3, escaneo de colisión + `consoleDrawText(1, 22, "FC ROW:%d COL:%d RSN:%u", ...)`) fue agregado para diagnosticar y ya cumplió su propósito — confirmado el resultado (RSN:2 = límite del tablero, no celda ocupada). Esta Story lo elimina como parte de su Task 2, no como una Story de limpieza separada.
- **Inventario de `consoleDrawText` a reubicar (columna actual → contenido, todas en `main.c`):**
  - `(6,2)` "Hello Apotris SNES" / `(3,4)` "PRESS A PAD BUTTON" — splash de boot, Story 1.1.
  - `(1,6)` "B:%u C:%u/%u/%u D:%u" — prueba `board_is_cell_occupied`, Stories 2.1/2.2/3.2.
  - `(1,8)` "PIECE: %u %u %d %d" — prueba `piece_spawn`, Story 3.3.
  - `(1,10)` "MOVE L:%d>%d R:%d>%d BLK:%d>%d" — prueba movimiento, Stories 3.4/3.7.
  - `(1,12)` "GRAV:%d>%d>%d>%d" — prueba gravedad, Stories 3.5/3.7.
  - `(1,14)` "LOCK X:%d Y:%d CNT:%u" — prueba lock, Stories 3.6/3.7.
  - `(1,18)` "LINES DET:%u ROW0:%u" / `(1,20)` "LINES COLLAPSE ROW10:%u CNT:%u" — prueba line-clear, Story 2.3.
  - `(6,16)` "X PRESSED" (varias variantes) — indicador de tecla real, Story 4.2 (este es el único que se sigue actualizando cada frame en el loop interactivo; el resto son prints de una sola vez durante el boot).
  - El bloque `TEMP DIAGNOSTIC` en `(1,22)` se elimina, no se reubica (Task 2).
- **Módulos y responsabilidad (`game-architecture.md#2`):** `render.c` es el único módulo que incluye headers de video (`background.h`/`oam.h`) y el único que escribe VRAM/OAM — no aplica a `consoleDrawText` (consola de texto de PVSnesLib, ya usada directamente desde `main.c` desde la Story 1.1, patrón existente sin cambios). Esta Story no mueve la consola a `render.c` — mantiene el patrón ya establecido.
- **Estrategia de VRAM (`game-architecture.md#8`):** el playfield (10×20, `SC_32x32`) pesa ~400 bytes — el reemplazo de arte no cambia esta estrategia, solo el contenido visual del tileset.
- **Memoria estática, sin alloc dinámica, C puro** (`project-context.md`) — el arte se genera/edita como asset (`.png` → `.pic` vía Makefile), no requiere código C nuevo más allá de mover argumentos de columna existentes.
- **Validación:** al finalizar, ejecutar únicamente `make` (no `make clean`) — la ROM debe quedar disponible para Ares. Secuencia completa: 1) `make`, 2) ejecutar en Ares, 3) esperar validación manual del usuario, 4) recién entonces considerar la Story terminada.
- **Anti-patrón a evitar:** no aprovechar esta Story para adelantar Stories futuras (line-clear real conectado a render, next-queue, HUD de score) — alcance mínimo fijado arriba.

### Project Structure Notes

- Archivos tocados: `snes/playfield.png` (y derivados regenerados por `make`: `.pic`/`.map`/`.inc`/`.pal`), `snes/source/main.c` (solo columnas de `consoleDrawText` + eliminación del bloque `TEMP DIAGNOSTIC`).
- Sin archivos nuevos, sin módulos nuevos.

### Project Context Rules

- Memoria estática, sin alloc dinámica (`project-context.md#Defaults de implementación`).
- C puro, sin C++/STL (`project-context.md#Stack técnico objetivo`).
- No implementar features fuera del alcance mínimo fijado arriba sin pedido explícito (`project-context.md#Anti-patrones para agentes`).
- Reutilizar arquitectura existente — sin funciones ni módulos nuevos para esta Story.
- Validación: solo `make` al final (no `make clean`) — ROM lista para Ares.

### References

- `_bmad-output/game-architecture.md#2 Módulos y responsabilidad, #8 Estrategia mínima de tilemap/VRAM`.
- `_bmad-output/project-context.md#Defaults de implementación, Anti-patrones para agentes`.
- `_bmad-output/implementation-artifacts/4-3-gravedad-automatica.md` (Story previa — gravedad/colisión validadas, board_init fix).
- `snes/source/main.c`, `snes/source/render.c`, `snes/playfield.png`, `snes/Makefile` (target `playfield.pic`) — estado actual leído completo para esta Story.
- `reference/apotris/sprites/05frameTexture.png`, `19frameTexture2.png`, `08barTexture.png` — solo referencia visual/de distribución.

## Dev Agent Record

### Agent Model Used

claude-sonnet-5

### Debug Log References

- `reference/apotris/sprites/05frameTexture.png`, `19frameTexture2.png`, `08barTexture.png` inspeccionados (paleta gris metálica de marco) — solo como inspiración de paleta/estilo, sin copiar píxeles ni leer código C++.
- Primer intento de regenerar `playfield.png` con PIL en modo RGB truecolor falló en `make`: `gfx4snes: fatal error: png decoder error 82: color conversion to palette requested while a color isn't in palette, or index out of bounds`. Diagnóstico: el `playfield.png` original está en modo `P` (paleta indexada, confirmado leyendo el PNG del commit `HEAD`), y `gfx4snes -p` espera ese formato. Fix: regenerar con `Image.quantize(palette=...)` para producir un PNG paleta-indexada de 2 colores, igual que el original.
- `make` (tras el fix de paleta): compiló y linkeó sin errores (ROM 90.53% libre, antes 90.15%).
- Verificado con `grep`: cero coincidencias de `TEMP DIAGNOSTIC`/`debug_collision_reported`/`debug_prev_y`/`debug_target_y`/`FC ROW` en `main.c`.
- Verificado ancho de cada string reubicado con un script Python usando valores numéricos de peor caso (ej. `-9`, `99`) contra columna de inicio 11 — dos strings (`MOVE ...BLK...` y `LINES COLLAPSE ROW10...CNT...`) excedían las 21 columnas disponibles y se partieron en dos filas (fila+1) cada uno; el resto entra en una sola fila.
- `git diff --stat` confirma que `board.c`, `piece.c`, `input.c`, `game_state.h`, `render.c` no aparecen en el diff — solo `snes/playfield.png` y `snes/source/main.c`.

### Completion Notes List

- **Arte del playfield:** `snes/playfield.png` reemplazado — mismo tamaño exacto (80×160px, 10×20 tiles de 8px), mismo modo paleta-indexada, 2 colores (relleno `(12,14,28)` + borde de acento `(96,104,128)`, 2px de grosor en los 4 bordes de la imagen completa, no por-celda). Sin rastro del grid plano de cuadros anterior. `bgInitTileSet`/`bgInitMapSet` en `main.c` sin cambios (mismos punteros/tamaños/`SC_32x32`/`BG_16COLORS`).
- **Instrumentación de la Story 4.3 eliminada:** bloque `TEMP DIAGNOSTIC` completo (variable, escaneo de colisión, `consoleDrawText` de fila 22) quitado de `main.c`. El gravity tick vuelve a su forma original de la Story 4.3 (solo contador + `piece_apply_gravity(&gs)` directo).
- **Texto de depuración reubicado:** las 19 llamadas `consoleDrawText` de `main.c` (splash de boot, pruebas de Stories 1.1–3.7, indicador de tecla de Story 4.2) movidas de columnas 1/3/6 a columna 11 (columna 10 como separador respecto al playfield de 10 columnas). Dos strings que no entraban en las 21 columnas disponibles (`MOVE L:...BLK:...` y `LINES COLLAPSE ROW10:...CNT:...`) se partieron en dos filas cada uno (fila original + fila siguiente, antes libres) — mismo contenido, sin cambiar qué los dispara. Ningún cambio a la lógica de las pruebas de boot (`piece_move_left/right`, `piece_apply_gravity`, `piece_lock`, `board_detect_full_lines`, `board_collapse_lines` siguen llamándose exactamente igual, mismo orden).
- **Sin cambios a `board.c`, `piece.c`, `input.c`, `game_state.h`, `render.c`** — confirmado por `git diff --stat`.
- **AC #8 (confirmación visual en Ares) pendiente del usuario** — no hay emulador en este entorno; el resto de ACs (#1–#7) verificados por revisión de código/build.

### File List

- `snes/playfield.png` (modificado) — arte definitivo (relleno + borde de acento), mismo tamaño/paleta-indexada que el placeholder anterior.
- `snes/source/main.c` (modificado) — bloque `TEMP DIAGNOSTIC` de la Story 4.3 eliminado; todas las llamadas `consoleDrawText` reubicadas a columna 11 (dos de ellas partidas en dos filas por ancho).

## Change Log

| Fecha | Cambio |
| --- | --- |
| 2026-07-12 | Implementación de Story 4.4 (layout definitivo del playfield): `snes/playfield.png` reemplazado por arte definitivo (relleno + borde de acento, inspirado visualmente en la paleta gris-metálica de los frames de Apotris, mismo tamaño/paleta-indexada). Bloque de instrumentación temporal de la Story 4.3 (`TEMP DIAGNOSTIC`) eliminado de `main.c`. Todas las llamadas `consoleDrawText` reubicadas a columna 11 (columna 10 como separador del playfield de 10 columnas); dos strings partidos en dos filas por ancho. Sin cambios a `board.c`/`piece.c`/`input.c`/`game_state.h`/`render.c`. `make` compila y linkea sin errores; `make clean` no ejecutado, ROM disponible para Ares. Status → review (confirmación visual en Ares pendiente del usuario). |
