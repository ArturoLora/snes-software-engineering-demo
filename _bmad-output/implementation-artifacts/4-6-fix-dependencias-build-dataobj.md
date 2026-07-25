---
baseline_commit: f23bfb42375b3e27f70d1006d583ac8d696e57f6
---

# Story 4.6: Fix dependencias de build — `data.obj` no se reensamblaba con assets nuevos

Status: review

## Nota de alcance

Story de infraestructura de build, solo `Makefile`. **Cero cambios** a `main.c`, `render.c`, `board.c`, `piece.c`, `input.c`, `game_state.h`, cualquier `.png`/asset, ni a `${PVSNESLIB_HOME}/devkitsnes/snes_rules` (incluido, no tocar). No repetir el diagnóstico — causa ya Confirmada en la revisión de la Story 4.5 (ver Dev Notes).

## Story

Como desarrollador del proyecto,
quiero que `data.obj` se reensamble automáticamente cuando cambie cualquier asset (`.pic`/`.map`/`.pal`) que `data.asm` incluye vía `.incbin`,
para que `make` nunca vuelva a enlazar una ROM con gráficos obsoletos en silencio.

## Acceptance Criteria

1. **Given** el diagnóstico ya Confirmado (revisión post-Story-4.5): `data.obj` depende solo del mtime de `data.asm` — Make no sabe que `data.asm` también incluye (`.incbin`) `playfield.pic`/`playfield.map`/`playfield.pal`/`pvsneslibfont.pic`/`pvsneslibfont.pal`/`piece.pic`, **when** se agrega una regla de prerequisitos explícitos (`data.obj: playfield.pic playfield.map playfield.pal pvsneslibfont.pic pvsneslibfont.pal piece.pic`) al `Makefile` del proyecto, **then** esa regla no reemplaza la receta de compilación existente (viene de `snes_rules`, no se toca) — solo agrega prerequisitos al mismo target.
2. **Given** el Makefile corregido, **when** se modifica únicamente `playfield.png` y se ejecuta `make`, **then** `data.obj` se reensambla automáticamente (mtime nuevo, más reciente que `playfield.png`) sin necesitar tocar `data.asm` a mano.
3. **Given** el mismo escenario, **when** `data.obj` se reensambla, **then** `apotris.sfc` también se re-linkea automáticamente (verificable por su mtime y por un cambio de hash si el asset cambió de tamaño).
4. **Given** el proyecto, **when** se ejecuta `make`, **then** compila y linkea sin errores (sin `make clean` — ROM lista para Ares).
5. **Given** `main.c`, `render.c`, `board.c`, `piece.c`, `input.c`, `game_state.h`, y cualquier `.png`, **when** la Story termina, **then** ninguno muestra diff — solo `Makefile`.

## Tasks / Subtasks

- [x] **Task 1: Declarar dependencias explícitas de `data.obj`** (AC: #1)
  - [x] En `snes/Makefile`, agregar una línea de solo-prerequisitos (sin receta) para `data.obj`, listando los archivos generados que `data.asm` incluye vía `.incbin`.
  - [x] Confirmar (leyendo `data.asm`) el listado exacto de `.incbin` — resultó ser **7**, no 6: `pvsneslibfont.pic`, `pvsneslibfont.pal`, `playfield.pic`, `playfield.map`, `playfield.pal`, `piece.pic`, `piece.pal` (la Story original omitía `piece.pal` — corregido al leer `data.asm` en vez de asumir de memoria).

- [x] **Task 2: Verificar la reconstrucción automática** (AC: #2, #3, #4)
  - [x] Tocado únicamente `playfield.png` (mtime, sin cambiar contenido — mismo md5 antes/después) y ejecutado `make`; confirmado por mtime y por el log de `make` (`wla-65816 ... -o data.obj data.asm` y `wlalink ...`) que `data.obj` y `apotris.sfc` se reconstruyen automáticamente sin tocar `data.asm`.
  - [x] `make` (build normal) — compila y linkea sin errores.
  - [x] `make clean` **no** ejecutado — ROM disponible para validación manual en Ares.

- [x] **Task 3: Confirmar alcance** (AC: #5)
  - [x] `git diff --stat` — solo `snes/Makefile` tiene diff atribuible a esta Story (ver nota en Completion Notes sobre `main.c`/`playfield.png`, ya modificados por Stories previas sin commitear).

## Dev Notes

- **Causa ya Confirmada, no repetir el diagnóstico:** durante la revisión posterior a la Story 4.5, se confirmó empíricamente (mtimes + `md5sum` antes/después de forzar `touch data.asm` + `make`) que `data.obj` no se había reensamblado desde antes de las Stories 4.4 y 4.5, pese a que `playfield.pic`/`.map`/`.pal` sí se regeneraban correctamente en cada `make`. `data.asm` (`snes/data.asm:15-24`) usa `.incbin` para embeber `playfield.pic`/`playfield.map`/`playfield.pal` (y `pvsneslibfont.pic`/`.pal`, `piece.pic`) — Make no interpreta `.incbin` (es sintaxis del ensamblador WLA-DX), así que la regla de compilación de `data.asm → data.obj` (definida en `snes_rules`, no en el Makefile del proyecto) solo mira el mtime de `data.asm` mismo. Como `data.asm` no cambia de contenido entre Stories, Make nunca vuelve a invocar el ensamblador, y `wlalink` embebe en la ROM el `data.obj` viejo — aunque el `.sfc` final sí se re-linkea y cambia de mtime (dando la falsa impresión de que el build "funcionó").
- **Por qué agregar solo prerequisitos, no una receta nueva:** GNU Make permite declarar el mismo target en más de una regla, siempre que como máximo una tenga receta — las demás solo aportan prerequisitos adicionales. `data.obj` ya tiene su receta de compilación en `snes_rules` (`${PVSNESLIB_HOME}/devkitsnes/snes_rules`, incluido por `snes/Makefile:7` — **no tocar**, per restricción de esta Story: "no cambios en PVSnesLib"). Agregar una línea `data.obj: playfield.pic playfield.map playfield.pal pvsneslibfont.pic pvsneslibfont.pal piece.pic` en `snes/Makefile` (junto a las reglas `*.pic: *.png` ya existentes) es el cambio mínimo: no redefine cómo se compila, solo le dice a Make que además de `data.asm`, `data.obj` depende de esos 6 archivos.
- **Los 6 `.incbin` exactos** (`snes/data.asm:15-24`): `pvsneslibfont.pic`/`pvsneslibfont.pal` (font), `playfield.pic`/`playfield.map`/`playfield.pal` (playfield), y (verificar en el archivo real al implementar) el `piece.pic` del sprite de la pieza activa — confirmar el listado completo leyendo `data.asm` antes de escribir la regla, no asumir de memoria.
- **No forzar reconstrucción con `touch data.asm` como workaround permanente** — eso fue el diagnóstico puntual de la revisión anterior, no la solución; la solución es la dependencia declarada en Task 1.
- **Memoria estática, C puro** (`project-context.md`) — el fix es de build, no de código de juego.
- **Validación:** al finalizar, ejecutar únicamente `make` (no `make clean`) — la ROM debe quedar disponible para Ares. Secuencia completa: 1) `make`, 2) ejecutar en Ares, 3) esperar validación manual del usuario, 4) recién entonces considerar la Story terminada. En este caso además: las Stories 4.4/4.5 quedan finalmente visibles en Ares recién con este fix — el usuario debería re-validar ambas al probar esta Story.

### Project Context Rules

- No investigar de nuevo lo ya Confirmado (`project-context.md#Investigación externa`).
- No modificar `PVSnesLib`/`snes_rules` — solo el `Makefile` del proyecto.
- Mantener el cambio lo más chico posible (una línea de prerequisitos).
- Validación: solo `make` al final (no `make clean`) — ROM lista para Ares.

### References

- `snes/data.asm:15-24` (los `.incbin` reales a listar como prerequisitos).
- `snes/Makefile:7` (`include ${PVSNESLIB_HOME}/devkitsnes/snes_rules` — no tocar).
- `snes/Makefile:21-37` (reglas `*.pic: *.png` ya existentes, mismo lugar donde agregar la nueva regla de prerequisitos).
- Revisión de la Story 4.5 (esta sesión) — evidencia empírica: mtimes de `data.obj`/`playfield.map`/`apotris.sfc` y `md5sum` de `apotris.sfc` antes/después de `touch data.asm` + `make`.

## Dev Agent Record

### Agent Model Used

claude-sonnet-5

### Debug Log References

- `grep -n ".incbin" data.asm` → 7 archivos (no 6 como decía el borrador de la Story): `pvsneslibfont.pic`/`.pal`, `playfield.pic`/`.map`/`.pal`, `piece.pic`/`.pal`. Se listaron los 7 en la nueva regla — ninguno de más ni de menos.
- Snapshot antes del fix: `data.asm` 22:16:53, `data.obj` 22:16:53, `apotris.sfc` 22:16:53 (md5 `df2b24d...`).
- Prueba de la Task 2: `touch playfield.png` (sin tocar contenido, mismo md5) + `make` → log muestra `gfx4snes` regenerando `playfield.pic/map/pal`, seguido de `wla-65816 -s -x -o data.obj data.asm` y `wlalink ... apotris.sfc` — **sin ninguna acción manual sobre `data.asm`**. `data.obj`/`apotris.sfc` avanzan a mtime 22:24:43, mientras `data.asm` queda en 22:16:53 (más viejo) — confirma que la dependencia agregada es la que dispara la reconstrucción, no un cambio en `data.asm`.
- `make` (build normal tras la prueba): `"No se hace nada para 'all'."` — ya estaba al día, sin errores.
- `git diff -- snes/Makefile`: diff de una sola regla nueva (8 líneas, comentario + prerequisitos), nada más tocado en el archivo.

### Completion Notes List

- **Fix aplicado:** una regla de solo-prerequisitos en `snes/Makefile` (después de `bitmaps :`): `data.obj: pvsneslibfont.pic pvsneslibfont.pal playfield.pic playfield.map playfield.pal piece.pic piece.pal`. No redefine la receta de compilación (sigue viniendo de `snes_rules`, sin tocar) — solo le dice a Make que `data.obj` también depende de esos 7 archivos, que antes eran invisibles para Make (solo `data.asm` los referencia vía `.incbin`, sintaxis que Make no interpreta).
- **Confirmado con evidencia de mtime + log de build**, no solo inspección de código: tocar solo `playfield.png` ahora dispara reensamblado de `data.obj` y re-link de la ROM automáticamente — antes (Stories 4.4/4.5) esto requería tocar `data.asm` a mano.
- **`data.asm`/`snes_rules`/código C/assets sin cambios** — único archivo tocado: `snes/Makefile`.
- **Nota sobre `git diff --stat` y el alcance real:** `main.c` y `playfield.png` también aparecen en el diff del repo, pero son cambios de las Stories 4.4/4.5 (sin commitear, mismo `baseline_commit`) — esta Story 4.6 no agregó ninguna línea a esos archivos (solo un `touch` para la prueba de la Task 2, que no altera contenido). El único diff atribuible a la Story 4.6 es el de `snes/Makefile`.
- **AC #6 (validación visual en Ares) pendiente del usuario** — con este fix, las Stories 4.4 y 4.5 deberían por fin ser visibles correctamente; recomendable re-validar ambas al probar esta Story.

### File List

- `snes/Makefile` (modificado) — agregada regla de solo-prerequisitos para `data.obj` (7 archivos `.incbin` de `data.asm`), sin tocar receta de compilación ni `snes_rules`.

## Change Log

| Fecha | Cambio |
| --- | --- |
| 2026-07-12 | Fix de build (Story 4.6): `snes/Makefile` declara `data.obj: pvsneslibfont.pic pvsneslibfont.pal playfield.pic playfield.map playfield.pal piece.pic piece.pal` como prerequisitos explícitos — Make no interpretaba los `.incbin` dentro de `data.asm`, así que `data.obj` solo se reensamblaba si `data.asm` mismo cambiaba, dejando la ROM enlazada con gráficos obsoletos pese a que `make` terminaba sin errores (causa raíz de que las Stories 4.4 y 4.5 no se vieran en Ares). Verificado tocando solo `playfield.png`: `data.obj`/`apotris.sfc` se reconstruyen automáticamente sin tocar `data.asm`. Sin cambios a `data.asm`, `snes_rules`, código C ni assets. `make` compila sin errores; `make clean` no ejecutado, ROM disponible para Ares. Status → review (validación visual pendiente del usuario — recomendable re-chequear también las Stories 4.4/4.5 con este fix). |
