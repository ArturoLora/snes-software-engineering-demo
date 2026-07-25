---
baseline_commit: f23bfb42375b3e27f70d1006d583ac8d696e57f6
---

# Story 4.5: Fix ancho del tilemap del playfield (32 tiles)

Status: review

## Nota de alcance

Story de corrección puntual, solo asset. **Cero cambios a código** (`main.c`, `render.c`, `board.c`, `piece.c`, `input.c`, `game_state.h` sin tocar). No repetir la investigación — causa ya diagnosticada en `_bmad-output/implementation-artifacts/investigations/bginitmapset-partial-tilemap-investigation.md`.

## Story

Como desarrollador del proyecto,
quiero que `playfield.png` genere un tilemap de 32 tiles de ancho,
para que el stride del `.map` coincida con `SC_32x32` y el playfield se vea confinado a su área real (10×20) en vez de desalinearse por toda la pantalla.

## Acceptance Criteria

1. **Given** el diagnóstico confirmado (investigación citada arriba: `gfx4snes -m` empaqueta el `.map` con stride = ancho en tiles de la imagen fuente; nuestro `playfield.png` mide 10 tiles de ancho, no 32, rompiendo la alineación con `SC_32x32`), **when** se reemplaza `snes/playfield.png` por un canvas de 256×160px (32×20 tiles), **then** el contenido real del playfield (relleno + borde de acento de la Story 4.4) ocupa las columnas 0-9, y las columnas 10-31 quedan en un tile de fondo vacío/plano.
2. **Given** el nuevo `playfield.png`, **when** se ejecuta `make`, **then** `playfield.pic`/`playfield.map`/`playfield.pal` se regeneran sin error y `main.c` no requiere ningún cambio (mismos punteros `playfieldtiles`/`playfieldmap`/`playfieldpal`, misma llamada `bgInitTileSet`/`bgInitMapSet`, mismo `SC_32x32`, misma dirección VRAM `0x7000`).
3. **Given** el proyecto, **when** se ejecuta `make`, **then** compila y linkea sin errores (sin `make clean` — ROM lista para Ares).
4. **Given** `main.c`, `render.c`, `board.c`, `piece.c`, `input.c`, `game_state.h`, **when** la Story termina, **then** ninguno muestra diff (`git diff --stat` los excluye).
5. **Given** la ROM corriendo en Ares, **when** el desarrollador observa el playfield, **then** el área de juego (columnas 0-9, filas 0-19) muestra el relleno + borde de acento de la Story 4.4 confinado a su rectángulo real, sin repetirse/desalinearse por el resto de la pantalla — confirmado manualmente por el usuario (no verificable por el agente).

## Tasks / Subtasks

- [x] **Task 1: Ensanchar el canvas de `playfield.png` a 32 tiles** (AC: #1)
  - [x] Regenerar `snes/playfield.png` a 256×160px (32×20 tiles): columnas 0-9 con el arte de la Story 4.4 (relleno `(12,14,28)` + borde de acento `(96,104,128)` de 2px en el perímetro del rectángulo 10×20 real, no del canvas completo), columnas 10-31 con un tile de fondo plano (ej. el mismo color de relleno, sin borde).
  - [x] Mantener la paleta paleta-indexada (`P` mode) igual que antes (requisito de `gfx4snes -p`, ya confirmado en la Story 4.4).

- [x] **Task 2: Build y verificación** (AC: #2, #3, #4)
  - [x] `make` — confirmar que `playfield.pic`/`.map`/`.pal` se regeneran sin error y que `main.c` sigue compilando sin cambios.
  - [x] `git diff --stat` — confirmar que solo `snes/playfield.png` aparece modificado (ningún `.c`/`.h`).
  - [x] `make clean` **no** ejecutado — ROM disponible para validación manual en Ares.

## Dev Notes

- **Causa ya diagnosticada, no repetir la investigación:** `_bmad-output/implementation-artifacts/investigations/bginitmapset-partial-tilemap-investigation.md` — Deducción 1. `gfx4snes -m` empaqueta el `.map` con stride = ancho en tiles de la imagen fuente (no relleno hasta 32 columnas). `SC_32x32` (pasado a `bgInitMapSet` en `main.c:108`) le dice al hardware que cada fila del nametable mide 32 tiles. Con un `playfield.png` de 10 tiles de ancho, el stride del `.map` (10) no coincide con el del hardware (32) → desalineamiento diagonal que dispersa el contenido por gran parte del nametable de 1024 tiles.
- **Por qué ensanchar el canvas y no tocar `main.c`:** el ejemplo oficial local `snes-examples/graphics/Backgrounds/Mode1Png/Mode1.c` prueba que un `mapSize` menor a 2048 bytes es válido (usa una imagen de 32×28 tiles, `mapSize` = tamaño real generado, no 2048 fijo) — el requisito real es que el ANCHO en tiles sea 32, no que el mapa sea completo. Ensanchar el PNG resuelve el stride sin tocar la llamada a `bgInitMapSet` (mismo `SC_32x32`, misma dirección `0x7000`).
- **Story 4.4 no se deshace:** el arte definitivo (relleno + borde de acento) de la Story 4.4 se conserva — solo se reubica dentro de un canvas más ancho, y las columnas 10-31 (fuera del área de juego real) quedan en un tile de fondo plano sin borde (para no confundir visualmente qué es el playfield real).
- **Memoria estática, C puro** (`project-context.md`) — el fix es un asset, no requiere código C nuevo.
- **Validación:** al finalizar, ejecutar únicamente `make` (no `make clean`) — la ROM debe quedar disponible para Ares. Secuencia completa: 1) `make`, 2) ejecutar en Ares, 3) esperar validación manual del usuario, 4) recién entonces considerar la Story terminada.
- **Anti-patrón a evitar:** no repetir la investigación de la causa (ya Confirmada/Deducida en el caso citado) ni adelantar trabajo de Stories futuras.

### Project Context Rules

- Memoria estática, sin alloc dinámica (`project-context.md#Defaults de implementación`).
- No investigar de nuevo lo ya descartado/diagnosticado (`project-context.md#Investigación externa`).
- Validación: solo `make` al final (no `make clean`) — ROM lista para Ares.

### References

- `_bmad-output/implementation-artifacts/investigations/bginitmapset-partial-tilemap-investigation.md` (causa raíz, Deducción 1 y Recommended Next Steps → Fix direction, ya cita este mismo cambio).
- `_bmad-output/implementation-artifacts/4-4-layout-playfield-definitivo.md` (arte a preservar: relleno + borde de acento).
- `snes/source/main.c:104-109` (llamadas `bgInitTileSet`/`bgInitMapSet`, sin cambios esperados).
- `snes-examples/graphics/Backgrounds/Mode1Png/Mode1.c` (ejemplo oficial local, citado en la investigación).

## Dev Agent Record

### Agent Model Used

claude-sonnet-5

### Debug Log References

- `make` (tras regenerar `playfield.png` a 256×160): salida de `gfx4snes` confirma directamente el fix — `"process image (256x160px, 256colors)"`, `"managed a map of 32x20 tiles of 8x8 pixels"`, `"saving map file [playfield.map] of (32x20) tiles with offset 0"`, `"9 tiles (ratio 99%) processed"`. Esto CONFIRMA (no solo Deduce) la causa de la investigación citada: con el canvas ensanchado, `gfx4snes -m` ahora empaqueta el `.map` con stride 32, coincidiendo con `SC_32x32`.
- `ls -la playfield.map` → 1280 bytes = 32×20×2 (tamaño esperado exacto para un mapa de 32 tiles de ancho).
- `make` (build completo, forzado con `touch source/main.c` para descartar cache): compiló y linkeó sin errores (ROM 90.53% libre, igual que la Story 4.4 — el asset no cambió el tamaño de ROM de forma apreciable).
- `git diff --stat` sobre `board.c`/`piece.c`/`input.c`/`game_state.h`/`render.c`: sin salida (cero diff), confirmado.

### Completion Notes List

- **Fix aplicado:** `snes/playfield.png` regenerado a 256×160px (32×20 tiles) — columnas 0-9 con el arte de la Story 4.4 (relleno + borde de acento alrededor del rectángulo real 10×20, no del canvas completo), columnas 10-31 en relleno plano sin borde. Paleta-indexada (`P` mode, 2 colores), igual convención que la Story 4.4.
- **Confirmado por la propia herramienta:** el log de `gfx4snes` (no solo inspección de tamaño de archivo) confirma que el `.map` generado ahora es de 32×20 tiles — la causa raíz de la investigación (`bginitmapset-partial-tilemap-investigation.md`) queda Confirmada, no solo Deducida.
- **`main.c` sin cambios de esta Story:** no se hizo ningún `Edit`/`Write` sobre `main.c` en esta Story — el único comando que lo tocó fue `touch` (cambia mtime, no contenido) para forzar un rebuild completo y descartar cache de `make`.
- **Nota sobre AC #4 y `git diff --stat`:** el AC #4 (redactado en la Story) listaba `main.c` entre los archivos que no deberían mostrar diff — pero `main.c` YA tenía cambios pendientes sin commitear de la Story 4.4 (bloque `TEMP DIAGNOSTIC` eliminado, `consoleDrawText` reubicados) desde antes de empezar esta Story 4.5, relativos al mismo `baseline_commit`. Esos 72+/24- de `git diff --stat` son 100% de la Story 4.4, no de esta — confirmado porque ningún `Edit`/`Write` tocó `main.c` en esta sesión. El AC debió excluir `main.c` de esa lista (o comparar contra el estado post-4.4, no contra `baseline_commit`). `render.c`, `board.c`, `piece.c`, `input.c`, `game_state.h` sí muestran cero diff, tal como pedía el alcance real ("Modificar únicamente el asset del playfield").
- **AC #5 (confirmación visual en Ares) pendiente del usuario** — no hay emulador en este entorno.

### File List

- `snes/playfield.png` (modificado) — canvas ensanchado a 256×160px (32×20 tiles); arte real de la Story 4.4 en columnas 0-9, relleno plano en columnas 10-31.

## Change Log

| Fecha | Cambio |
| --- | --- |
| 2026-07-12 | Implementación de Story 4.5 (fix ancho tilemap playfield): `snes/playfield.png` ensanchado de 80×160px (10 tiles) a 256×160px (32 tiles), confinando el arte real de la Story 4.4 a las columnas 0-9 y dejando relleno plano en las columnas 10-31. `gfx4snes` confirma en su propio log que el `.map` generado ahora es de 32×20 tiles (antes 10×20), resolviendo el desalineamiento de stride diagnosticado en la investigación previa. Sin cambios a ningún archivo `.c`/`.h` en esta Story. `make` compila y linkea sin errores; `make clean` no ejecutado, ROM disponible para Ares. Status → review (confirmación visual en Ares pendiente del usuario). |
