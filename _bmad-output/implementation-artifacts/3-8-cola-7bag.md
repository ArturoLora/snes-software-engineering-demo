---
baseline_commit: 2841a40
baseline_dirty:
  - path: CLAUDE.md
    md5: 8e09f510c13d87b189d3ebe77a05149a
  - path: README.md
    md5: 0761a1453e6dd8a9841bd478c9d9c2d3
  - path: _bmad-output/implementation-artifacts/process-1-baseline-limpio-y-v0-reproducible.md
    md5: 46c33acc709849c0136f000502a2bfed
  - path: _bmad-output/project-context.md
    md5: c5571b8f9844198fdd6f301553f65e78
  - path: docs/BMAD_GAMEDEV_NATIVE_LOOP.md
    md5: eec6a75c7a59a31a8c52f90005201b83
  - path: pantalla.txt
    md5: 572f2338a7c81869489608fd3beb6376
  - path: tools/loop/story_baseline.py
    md5: 5e04519372f9d4f72bfef70efbae73b9
story_class: algoritmos
minimum_validation: V0 + V1
review_level: A
---

# Story 3.8 — Cola de siguiente pieza (7-bag)

**Status:** done — V0 y V1 en PASS. Los 7 AC verificados. Sin V2 por clase

> Corresponde a **Story 3.2 de `epics.md`**, nunca ejecutada. El archivo va como
> `3-8` porque el Epic 3 ya llegó a `3-7` — ver `BOOTSTRAP.md` §3.

## Story

Como jugador,
quiero que salgan las siete piezas y no siempre la misma,
para que el juego sea jugable en lugar de una sucesión infinita de piezas I.

## Estado de partida

`piece_spawn()` hace `gs->piece.type = 0`, fijo, desde la Story 3.3. Las siete
formas existen en `piece_shapes[]` desde la 3.2 y **nunca se han usado**: el render
de sprites y el bake del tablero ya son genéricos sobre la forma, así que ninguno
necesita cambios.

## Clasificación

| Campo | Valor |
|---|---|
| **Clase** | `algoritmos` — la tabla la nombra literalmente: "RNG / bag" |
| **Nivel mínimo** | V0 + V1. Sin V2 |
| **Nivel de Review** | **A**, por escalado mecánico |
| **Derivación de la Review** | La clase `algoritmos` daría nivel B. Sube a **A** por el disparador nº1: la Story **añade estado en WRAM** (`PieceQueue` en `GameState`, más el módulo nuevo), y eso desplaza el layout. Es exactamente lo que produjo G1 en la Story 4-8 |

**Por qué no exige V2:** el jugador verá piezas distintas, pero eso no es una
dimensión perceptual nueva — es la misma pieza renderizada con otra forma, por un
camino de render que ya se validó en V2 en las Stories 4.1 y 4.8. Lo que esta Story
introduce es una regla, y una regla se comprueba por consola, que es justo lo que
pide el AC del roadmap.

## Allowlist

- `snes/source/queue.c`, `snes/source/queue.h` (nuevos)
- `snes/source/game_state.h`
- `snes/source/piece.c`
- `snes/source/main.c`
- `_bmad-output/implementation-artifacts/3-8-cola-7bag.md`

Fuera: `board.*`, `render.*`, `input.*`, `piece_data.*`, `test_*`, el `Makefile`,
todos los assets, y todo `tools/` y `docs/`.

## Acceptance Criteria

1. **AC1** — Cada grupo de 7 piezas consecutivas contiene **cada uno de los 7 tipos
   exactamente una vez**. Sin repeticiones ni ausencias dentro de una bolsa.
2. **AC2** — Al vaciarse la bolsa se rellena y se baraja de nuevo, indefinidamente.
   No hay número máximo de bolsas.
3. **AC3** — `piece_spawn()` toma el tipo de la cola. Ningún otro módulo decide qué
   pieza sale.
4. **AC4** — La secuencia **varía entre ejecuciones** a partir de la primera bolsa.
   Ver el límite declarado abajo: la primera bolsa es determinista y eso se registra
   como diferido, no se disimula.
5. **AC5** — El estado de la cola vive en `GameState`, **añadido después de
   `lines`**, para no mover los desplazamientos de `piece` dentro de la estructura.
   El script de V1 los tiene fijos y moverlos rompería la medición (G1).
6. **AC6** — V1 sigue en `PASS` y la cadencia de gravedad sigue siendo de 30 frames.
   Guarda de regresión: una Story de RNG no puede mover la temporización.
7. **AC7** — El conjunto atribuible no toca `board.*`, `render.*` ni ningún asset.

## Tasks / Subtasks

- [x] **T1 — `PieceQueue` en `game_state.h`** (AC5)
  - `bag[7]` con los tipos pendientes y `bag_count`. **Al final del struct**, después
    de `lines`.
- [x] **T2 — `queue.c` / `queue.h`** (AC1, AC2, AC4)
  - `queue_init()`: deja la bolsa vacía; el primer relleno es perezoso.
  - `queue_next()`: si la bolsa está vacía la rellena con los 7 tipos y la baraja
    (Fisher-Yates con `rand()`); devuelve y consume el siguiente tipo.
  - No incluye headers de vídeo ni de input.
- [x] **T3 — `piece_spawn()` consume la cola** (AC3)
  - Sustituir `type = 0` por `type = queue_next(gs)`. Nada más cambia.
- [x] **T4 — Arranque y avance del RNG en `main.c`** (AC4)
  - `queue_init()` antes del bucle.
  - Consumir un `rand()` por frame en el bucle, para que el momento del relleno de
    cada bolsa dependa del juego y no de un número de frame fijo.
  - Las pruebas de arranque previas a `setScreenOn()` fijan `type = 0` justo después
    de su `piece_spawn()`: fueron escritas para la geometría de la pieza I y con un
    tipo aleatorio dejarían de comprobar lo que dicen comprobar.
  - Añadir una línea de consola con la secuencia de tipos generados, que es la
    evidencia que pide el AC del roadmap.
- [x] **T5 — V0** (AC7)
- [x] **T6 — V1** (AC6)
- [x] **T7 — Contención de alcance y comprobación del contrato de direcciones**
  - `story_baseline.py check`.
  - Comprobar en `apotris.sym` que `pad0` y `gs` siguen donde el script Lua de V1
    los busca. Si se movieron, V1 mide basura — G1 otra vez.

## Dev Notes

- **PVSnesLib no expone `srand()`.** Verificado en
  `$PVSNESLIB_HOME/pvsneslib/include/`: solo hay `u16 rand(void)`. La secuencia del
  generador es la misma desde el reset, así que la única forma de variar es
  **consumir una cantidad distinta de valores**. De ahí el `rand()` por frame y el
  relleno perezoso: a partir de la segunda bolsa, el instante del relleno depende de
  cuándo aterrizó la pieza anterior, que depende del jugador.
- **Límite declarado: la primera bolsa es determinista.** Se rellena antes de que el
  jugador haya podido tocar nada, así que las primeras 7 piezas serán las mismas en
  cada arranque. Cerrarlo exige una fuente de entropía real —lo habitual es sembrar
  con `snes_vblank_count` en una pantalla de título, al primer botón—, y aquí no hay
  pantalla de título. **Se registra como diferido, no se implementa.**
- No se implementa la cola de vista previa (`queue[N]` de la arquitectura): mostrar
  la pieza siguiente es HUD, no está en el alcance mínimo, y la propia arquitectura
  lo marca como no exigido por el MVP.

## Dev Agent Record

### Completion Notes

Bolsa de 7 con Fisher-Yates en `queue.c`, consumida desde el final. `piece_spawn()`
es el único consumidor. `PieceQueue` va al final de `GameState`, y el contrato de
direcciones de V1 sobrevivió intacto: `pad0` en `$7E2805` y `gs` en `$7E2807`, los
mismos que busca el script Lua. `test_status` sí se desplazó a `$7E28FC`, pero nada
activo lo lee por dirección fija.

## Review independiente — nivel A

Escalada mecánica desde B por el disparador nº1 (estado nuevo en WRAM).
**Veredicto: Cambios requeridos.** Cinco hallazgos: dos `mayor`, tres `menor`.

El algoritmo salió indemne. El revisor enumeró **las 5040 secuencias de decisión
posibles** del Fisher-Yates y obtuvo las 5040 permutaciones, cada una exactamente una
vez — distribución uniforme, ningún tipo excluido de ninguna posición. Y lo contrastó
en emulador sobre **22 bolsas consecutivas, cero inválidas**.

Lo que falló fue **el artefacto de evidencia**, no la lógica.

| # | Hallazgo | Severidad | Disposición |
|---|---|---|---|
| H1 | **Este toolchain no limpia BSS.** `spawn_log_n` medido leyendo `0x55` en WRAM real durante 1100 frames, sin cambiar nunca. Al ser 85 ≥ 7, la rama de llenado inicial era código muerto y la línea imprimía `85` en las ranuras vacías. Como `consoleDrawText` no rellena con espacios, dejaba glifos obsoletos permanentes al encogerse la cadena | `mayor` | `patch` — **resuelto** |
| H2 | La línea `BAG` era una **ventana rodante**, no alineada a bolsa. Un grupo de 7 que cruza frontera de bolsa no es una permutación: medido, **el 79 % de las ventanas** no lo era. Quien mirase la pantalla vería el AC incumplido cuatro de cada cinco veces con el código perfectamente correcto | `mayor` | `patch` — **resuelto** |
| H3 | Las pruebas de arranque queman 2 tiradas reales, así que la partida empezaba **a mitad de bolsa**: el primer grupo visible tenía 5 piezas | `menor` | `patch` — **resuelto** |
| H4 | Registro de ejecución de la Story sin rellenar | `menor` (proceso) | `patch` — resuelto con este texto |
| H5 | `queue.c` incluye `<snes.h>`, que arrastra headers de vídeo. Patrón preexistente en todo el proyecto, no regresión de esta Story | `menor` | `defer` |

### Correcciones aplicadas

- **H1 + H2 con un solo mecanismo.** La línea pasa a mostrar **la bolsa actual**, no
  las últimas siete piezas. La alineación se detecta con `bag_count`: tras la primera
  extracción de una bolsa nueva quedan exactamente 6. Las ranuras aún no sacadas
  llevan el centinela `9` —un dígito, y nunca un tipo válido—, así que la línea tiene
  **ancho fijo** y no deja rastro, y no depende de que BSS esté a cero.
- **H3.** `queue_init()` extra justo antes del spawn del bucle vivo, para que la
  partida arranque en frontera de bolsa.

### Lo que el revisor intentó romper y aguantó

Fisher-Yates por enumeración exhaustiva · aritmética `u8` (`i+1` promociona a `int`,
`i` nunca baja de 1, `j` siempre dentro del array) · orden de consumo desde el final ·
**corrupción de memoria por el `consoleDrawText` de 7 varargs** —era su sospecha
principal; instrumentó `pad0`, todo `GameState` y `test_status` durante 161 llamadas
sin un byte fuera de sitio— · encaje en pantalla · y el contrato de direcciones de V1,
que no se movió: **G1 no se repitió**.

### Re-validación tras los patches

V0 `Build finished successfully !` · V1 `PASS`, 13 lecturas, cadencia de gravedad
30 frames (66/96/126/156/186) · `pad0` y `gs` sin moverse · atribuible = 6 =
allowlist · ROM `3bc55c40…`.

## File List

- `snes/source/queue.c`, `snes/source/queue.h` — nuevos
- `snes/source/game_state.h` — `PieceQueue`, al final del struct
- `snes/source/piece.c` — `piece_spawn()` consume la cola
- `snes/source/main.c` — arranque de la cola, `rand()` por frame, línea `BAG`
- `_bmad-output/implementation-artifacts/3-8-cola-7bag.md` — nuevo

## Registro de validación

| Nivel | Estado | Evidencia |
|---|---|---|
| V0 | **PASS** | `make` → `Build finished successfully !`. ROM `3bc55c40…`. `rebuild_v0.py` del revisor: reproducible byte a byte, árbol intacto |
| V1 | **PASS** | exit `0`, `WRAM`, 13 lecturas, frames 0→186. Cadencia de gravedad 30 frames — sin regresión. La pieza viva sale con `type=4`, ya no fija en 0 |
| V2 | **no exigida** por la clase `algoritmos` | Ver deuda abajo |

### Deuda de validación manual

Esta Story se cierra sin V2. La clase `algoritmos` no la exige, y el camino de render
que dibuja las piezas nuevas ya se validó en V2 en las Stories 4.1 y 4.8 — lo que esta
Story añade es qué tipo sale, no cómo se dibuja. **A comprobar en la próxima sesión de
emulador:** que las siete formas se dibujan correctamente y que la línea `BAG` muestra
siete dígitos distintos al completarse cada bolsa.

### Hallazgos diferidos

- **D1 — BSS no se inicializa a cero en este toolchain.** Medido en WRAM real. Es un
  hecho del proyecto, no de esta Story: **toda variable `static` debe inicializarse
  explícitamente antes de usarse.** `render.c` ya lo hace en `render_board_init()`, así
  que no hay defecto abierto hoy, pero conviene que quede escrito. Candidato a
  `project-context.md`.
- **D2 — V1 nunca ejercita la ruta lock → spawn.** 186 frames a 30 por paso no bastan
  para que la pieza aterrice, así que el harness dio PASS sin tocar el código nuevo.
  Es la misma brecha que H8 de la Story 4-8.
- **D3 — El comentario de layout de `tools/lua/poc_read_memory.lua` quedó obsoleto**:
  dice 229 bytes y siguiente símbolo en `$7E28EC`; ahora son 237 y `$7E28F4`. Fuera de
  la allowlist. Insumo para la Story de G1.
- **D4 (H5)** — `queue.c` incluye `<snes.h>`, que arrastra headers de vídeo. Patrón
  preexistente en todo el proyecto.

## Change Log

| Fecha | Cambio |
|---|---|
| 2026-07-25 | Story creada (Create Story, Sprint autónomo) |
| 2026-07-25 | Implementación T1–T7. V0 y V1 en PASS, contrato de direcciones de V1 verificado intacto |
| 2026-07-25 | Review nivel A: Cambios requeridos. H1/H2/H3/H4 resueltos por `patch`, H5 `defer`. Re-validado. Story cerrada en `done` |
