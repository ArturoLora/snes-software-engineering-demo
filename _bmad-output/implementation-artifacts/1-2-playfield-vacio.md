---
baseline_commit: e4e50a797c7646da66e0852e344d448ae2d606be
---

# Story 1.2: Inicializar y mostrar el playfield (BG vacío)

Status: in-progress (implementación + build verificados; confirmación visual en emulador pendiente del usuario)

## Story

Como desarrollador,
quiero inicializar un segundo background con un tileset/mapa fijo de 10×20 celdas,
para ver el playfield vacío en pantalla como base sobre la que se dibujará el juego, antes de escribir cualquier lógica de tablero.

## Alcance (fijado explícitamente por el usuario)

**Sí implementar:**
- Inicializar el BG del playfield (`bgInitTileSet` + `bgInitMapSet`, confirmado en `snes/include/snes/background.h` de la instalación local de PVSnesLib).
- Asset estático mínimo: 1-2 tiles (celda vacía, opcionalmente borde) + un mapa fijo de 10×20 rellenado en tiempo de compilación (no en runtime).
- Mostrar el playfield vacío en pantalla.
- Mantener el texto de depuración de la Story 1.1 (consola + lectura de pad) si no choca con el nuevo BG.
- Integrar la inicialización del playfield al `main.c`/loop ya existente (init una sola vez; el loop principal no cambia su lógica).

**NO implementar todavía** (explícitamente fuera de alcance, aunque `game-architecture.md` las describa para stories futuras):
tablero lógico (`BoardState`/`board[][]`), piezas, colisiones, gravedad, input adicional, DMA de actualización/buffer espejo en WRAM, sprites, OAM, rotación, 7-bag. El playfield de esta story es un dibujo estático fijo — no se escribe ni se lee desde ningún estado de juego.

## Acceptance Criteria

1. **Given** la ROM de la Story 1.1 ya arranca correctamente, **when** se inicializa el BG del playfield con `bgInitTileSet`/`bgInitMapSet`, **then** se ve en pantalla la rejilla/fondo del playfield de 10×20 celdas.
2. **Given** el modo de video configurado, **when** arranca la ROM, **then** ningún BG no usado queda habilitado (`bgSetDisable` sobre el/los BG sin asignar).
3. **Given** el BG de consola de texto de la Story 1.1, **when** se agrega el BG del playfield, **then** ambos son visibles simultáneamente sin corromperse mutuamente (capas de VRAM/tilemap distintas).
4. **Given** el `main.c` de la Story 1.1, **when** se agrega la inicialización del playfield, **then** el loop principal (`while(1) { ...; WaitForVBlank(); }`) sigue teniendo una sola llamada a `WaitForVBlank()` por vuelta — el playfield se inicializa una vez antes del loop, no dentro de él.

## Tasks / Subtasks

- [x] **Task 1: Asset estático del playfield** (AC: #1)
  - [x] `snes/playfield.png` (80×160px = 10×20 celdas de 8×8, generado con PIL localmente): celda con borde de 1px (índice 1) y relleno (índice 0). Convertido con `gfx4snes -s 8 -o 16 -u 16 -e 0 -p -m -i` — la flag `-m` genera además el `.map`, así el mapa 10×20 sale directo del PNG en vez de escribirse a mano en asm.
  - [x] El mapa fijo de 10×20 sale de la conversión (`playfield.map`, 400 bytes = 200 celdas × 2 bytes) — confirmado por tamaño de archivo tras `make`. El tile se dedupe a 1 único tile (`playfield.pic` = 32 bytes = 1 tile 4bpp) porque las 200 celdas son idénticas.

- [x] **Task 2: Init del BG del playfield en `main.c`** (AC: #1, #2, #3)
  - [x] Playfield asignado a BG1 (BG0 sigue siendo la consola de texto de la Story 1.1).
  - [x] `bgInitTileSet(1, ...)` + `bgInitMapSet(1, ...)` (patrón confirmado localmente contra `snes-examples/graphics/Backgrounds/Mode1Png/Mode1.c` del PVSnesLib del usuario — mismo uso de `(&x_end - &x)` para `tileSize`/`paletteSize`/`mapSize`). `paletteEntry` = 1 (no 0) para no compartir el slot de paleta 0 con la fuente de la consola.
  - [x] `bgSetEnable(1)` agregado; `bgSetDisable(1)` de la Story 1.1 se retiró (BG1 ya no está sin usar) y `bgSetDisable(2)` se mantiene (BG2 sigue sin uso).
  - [x] Init ocurre una sola vez antes de `setScreenOn()`, fuera del `while(1)` — el loop no se tocó.

- [x] **Task 3: Compilar y verificar en emulador** (AC: #1, #2, #3, #4)
  - [x] `make` en `snes/` (con el PVSnesLib local del usuario en `~/Tools/pvsneslib`) → compila y linkea sin errores, `snes/apotris.sfc` generado. `make clean` corrido al final.
  - [ ] Confirmación visual en emulador — **pendiente del usuario** (no hay emulador en este entorno de agente).

## Dev Notes

- **API confirmada localmente** (no inventada): `bgInitTileSet(u8 bgNumber, u8 *tileSource, u8 *tilePalette, u8 paletteEntry, u16 tileSize, u16 paletteSize, u16 colorMode, u16 address)` y `bgInitMapSet(u8 bgNumber, u8 *mapSource, u16 mapSize, u8 sizeMode, u16 address)` — verificadas en `devkitsnes`/`pvsneslib/include/snes/background.h` de la instalación local del usuario (`~/Tools/pvsneslib`). `bgSetEnable`/`bgSetDisable` también confirmadas ahí.
- La Story 1.1 usa BG0 para la consola de texto (`consoleSetTextMapPtr(0x6800)`, `bgSetGfxPtr(0, 0x2000)`, `bgSetMapPtr(0, 0x6800, SC_32x32)`). El playfield en BG1 debe usar direcciones de VRAM distintas para tileset/tilemap (no pisar `0x2000`/`0x6800`) — elegir offsets libres (ej. `0x4000` para tiles, `0x7000` para el mapa; ajustar según el tamaño real del asset generado por `GFXCONV`).
- Sin DMA ni buffer espejo en WRAM en esta story — eso es de Epic 4 (`game-architecture.md` §8, Story 4.3). Acá el mapa se escribe una sola vez al boot y no vuelve a tocarse.
- Sin `board.c`/`game_state.h` todavía — no hay Epic 2 implementado. El "playfield vacío" de esta story es solo el BG visual, no una estructura de datos de juego.
- Mantener `snes/source/main.c` como único archivo — no crear `render.c` todavía (ese módulo, con ownership exclusivo de VRAM/OAM, es de Epic 4 según `game-architecture.md` §7). Esta story es boot puro, igual que la 1.1.

### References

- `_bmad-output/planning-artifacts/epics.md#Story 1.2: Inicializar y mostrar el playfield (BG vacío)`
- `_bmad-output/game-architecture.md#8 Estrategia mínima de tilemap/VRAM`, `#9 Inicialización mínima de ROM/PVSnesLib`
- `_bmad-output/project-context.md#Alcance mínimo, Anti-patrones para agentes`
- `_bmad-output/implementation-artifacts/1-1-boot-infraestructura-minima.md` (boot previo, BG0/consola de texto ya init)
- API confirmada por inspección local: `~/Tools/pvsneslib/pvsneslib/include/snes/background.h`

## Dev Agent Record

### Agent Model Used

claude-sonnet-5

### Debug Log References

- `make` en `snes/` (PVSNESLIB_HOME=~/Tools/pvsneslib): build limpio, sin errores/warnings de link. `playfield.pic`=32B (1 tile deduplicado), `playfield.map`=400B (200 celdas), `playfield.pal`=32B — tamaños esperados verificados antes de correr `make clean`.

### Completion Notes List

- **Direcciones VRAM elegidas** para no chocar con la Story 1.1 (BG0: tiles@0x2000, mapa@0x6800, tamaño 0x800): playfield BG1 tiles@0x4000, mapa@0x7000 (justo después del bloque de mapa de BG0). `paletteEntry=1` para que el playfield tenga su propio banco de 16 colores en CGRAM, separado del de la fuente de consola (`paletteEntry=0`).
- **Decisión de asset:** en vez de escribir el mapa 10×20 a mano en `data.asm`, se generó una imagen PNG de 80×160 (10×20 celdas de 8×8) con una celda repetida y se usó la flag `-m` de `gfx4snes` para que el propio conversor produzca el `.map` — coherente con "no tablero lógico todavía": el layout es un dato estático de compilación, no una estructura en runtime.
- **Patrón de código confirmado localmente, no inventado:** `bgInitTileSet`/`bgInitMapSet` con `(&simbolo_end - &simbolo)` para los tamaños se copió del ejemplo oficial `snes-examples/graphics/Backgrounds/Mode1Png/Mode1.c` presente en la instalación local de PVSnesLib del usuario (sin consultar Internet, según lo pedido).
- Se quitó `bgSetDisable(1)` de la Story 1.1 (BG1 ahora está en uso) y se agregó `bgSetEnable(1)`; el resto del `main.c` de la Story 1.1 (consola de texto, lectura de pad, loop) no se modificó en su lógica, solo se le insertó el bloque de init de BG1 antes de `setMode`.
- **No verificado por el agente:** el resultado visual en emulador (AC #1-#4 son "se ve en pantalla...") — no hay emulador disponible en este entorno. El usuario ya validó el toolchain con `ares` en la Story 1.1; falta repetir esa validación visual para esta story.

### File List

- `snes/playfield.png` (nuevo) — asset fuente: 10×20 celdas de 8×8 con borde de 1px, generado localmente con PIL.
- `snes/Makefile` (modificado) — regla `playfield.pic: playfield.png` (con `-m` para generar también `.map`), agregada a `bitmaps`.
- `snes/data.asm` (modificado) — nueva sección `.rodata2` con símbolos `playfieldtiles`/`playfieldmap`/`playfieldpal` (+ `_end`) para `playfield.pic`/`.map`/`.pal`. La sección de la fuente de consola (Story 1.1) no se tocó.
- `snes/source/main.c` (modificado) — agregado init de BG1 (`bgInitTileSet`+`bgInitMapSet`+`bgSetEnable(1)`) antes de `setMode`/`setScreenOn`; quitado `bgSetDisable(1)` (ya no aplica). Loop principal y lectura de pad de la Story 1.1 sin cambios.

### Change Log

| Fecha | Cambio |
| --- | --- |
| 2026-07-11 | Implementación de Story 1.2: BG1 inicializado con tileset+mapa estático de playfield 10×20 (asset generado localmente, sin tablero lógico). `make` compila sin errores. Status → in-progress (confirmación visual en emulador pendiente del usuario). |
