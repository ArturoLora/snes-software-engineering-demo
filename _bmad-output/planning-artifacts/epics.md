---
stepsCompleted: [1, 2, 3, 4]
inputDocuments:
  - '_bmad-output/project-context.md'
  - '_bmad-output/deep-dive-tetris-core.md'
  - '_bmad-output/game-architecture.md'
  - '_bmad-output/implementation-artifacts/investigations/pvsneslib-mvp-cross-deps-investigation.md'
  - '_bmad-output/implementation-artifacts/investigations/pvsneslib-tilemap-runtime-update-investigation.md'
status: 'complete'
---

# apotris-snes - Epic Breakdown

## Overview

No existe GDD formal para este proyecto (brownfield SNES, alcance fijado directamente por el usuario en `project-context.md`). Este documento usa esa lista de alcance como sustituto funcional de los FRs, y las reglas técnicas de `project-context.md`/`game-architecture.md` como NFRs y requisitos adicionales.

## Requirements Inventory

### Functional Requirements

FR1: El sistema debe arrancar la ROM e inicializar PVSnesLib sin fallos.
FR2: El sistema debe mostrar un playfield (tablero) en pantalla.
FR3: El sistema debe generar una pieza activa (spawn).
FR4: El jugador debe poder mover la pieza activa horizontalmente.
FR5: La pieza activa debe caer automáticamente por gravedad (curva por nivel, Q8.8).
FR6: El sistema debe detectar colisión entre la pieza activa, los bordes del tablero y las celdas ocupadas.
FR7: La pieza activa debe fijarse (lock) al tablero al aterrizar.
FR8: El sistema debe generar la siguiente pieza mediante una cola (7-bag).
FR9: El sistema debe detectar y colapsar filas completas (line clear) tras un lock.
FR10: El sistema debe renderizar el tablero y la pieza activa en pantalla (BG + OAM).
FR11: El sistema debe leer input real del pad de PVSnesLib.
FR12: El sistema debe reflejar en VRAM (vía DMA) los cambios del tablero al hacer lock/line-clear.

### NonFunctional Requirements

NFR1: Sin asignación dinámica de memoria — todas las estructuras de tamaño fijo (`project-context.md`).
NFR2: C puro, sin C++ ni STL — el toolchain (816-tcc) no los soporta (`pvsneslib-mvp-cross-deps-investigation.md`).
NFR3: Sin ASM 65C816 salvo cuello de botella medido — ninguna story de este MVP lo requiere.
NFR4: RNG vía `rand()` de PVSnesLib (`console.h`), sin capa de reseed manual.
NFR5: Tipos `u8`/`u16` de `snestypes.h`.
NFR6: Gravedad en fixed-point Q8.8 (`u16`), tablas convertidas offline desde los valores float de Apotris.
NFR7: Lógica de gameplay (board/piece/queue) sin dependencia de headers de video de PVSnesLib — `render.c` es el único módulo con VRAM/OAM.

### Additional Requirements (de `game-architecture.md`)

- Módulos y ownership: `board.c` (tablero+colisión+líneas), `piece.c` (pieza activa: spawn/movimiento/gravedad/lock), `queue.c` (bag/siguiente pieza), `input.c` (única frontera con pads), `render.c` (único módulo con VRAM/OAM), `piece_data.c` (tablas estáticas), `game_state.h` (contrato de structs).
- Orden de implementación recomendado: `game_state.h` → `board.c` → `piece_data.c` → `piece.c` → `queue.c` → frame loop lógico validado por consola de texto (`consoleInitText`) sin render → `input.c` → `render.c`.
- Playfield: 1 BG (`bgInitTileSet`/`bgInitMapSet`), tilemap actualizado solo en lock/line-clear vía buffer WRAM espejo + `dmaCopyVram` tras `WaitForVBlank()` — transferencia completa del playfield, no celda-a-celda (`pvsneslib-tilemap-runtime-update-investigation.md`).
- Pieza activa: hasta 4 sprites OAM (`oamSet`/`oamSetXY`), repositionados cada frame que se mueve; ocultos (`oamSetVisible`) tras el lock.

### UX Design Requirements

N/A — no hay documento de UX; no aplica a este MVP (sin menús/UI más allá del playfield).

### FR Coverage Map

| FR   | Cubierto por |
| ---- | ------------ |
| FR1  | Epic 1 / Story 1.1 |
| FR2  | Epic 1 / Story 1.2 |
| FR3  | Epic 3 / Story 3.1 |
| FR4  | Epic 3 / Story 3.3 |
| FR5  | Epic 3 / Story 3.4 |
| FR6  | Epic 2 / Story 2.2 |
| FR7  | Epic 3 / Story 3.5 |
| FR8  | Epic 3 / Story 3.2 |
| FR9  | Epic 2 / Story 2.3, Epic 3 / Story 3.6 |
| FR10 | Epic 4 / Story 4.1 |
| FR11 | Epic 4 / Story 4.2 |
| FR12 | Epic 4 / Story 4.3 |

## Epic List

1. Boot y fundación de render
2. Tablero y colisión (lógica pura)
3. Ciclo de vida de la pieza activa (lógica pura)
4. Integración de render, input y DMA

---

## Epic 1: Boot y fundación de render

Arrancar la ROM en hardware/emulador real y mostrar el playfield vacío en pantalla — primera evidencia visual de que el proyecto corre en SNES.

### Story 1.1: Boot de ROM e inicialización mínima de PVSnesLib

Como desarrollador,
quiero que la ROM arranque e inicialice PVSnesLib (modo de video, pantalla activa),
para tener una base ejecutable verificable en hardware/emulador antes de escribir cualquier lógica de juego.

**Acceptance Criteria:**

**Given** la ROM compilada se carga en un emulador o hardware SNES real
**When** arranca
**Then** no crashea ni cuelga
**And** llega a un estado visual definido (pantalla activada, color de fondo sólido) confirmado con `setScreenOn()`

### Story 1.2: Inicializar y mostrar el playfield (BG vacío)

Como desarrollador,
quiero inicializar el BG del playfield y cargar su tileset/mapa,
para ver el tablero vacío en pantalla como base sobre la que se dibujará el juego.

**Acceptance Criteria:**

**Given** la ROM de la Story 1.1 ya arranca correctamente
**When** se inicializa el BG del playfield con `bgInitTileSet`/`bgInitMapSet`
**Then** se ve en pantalla la rejilla/fondo del playfield de 10×20 celdas
**And** ningún otro BG no usado queda habilitado (`bgSetDisable`)

---

## Epic 2: Tablero y colisión (lógica pura)

Implementar el tablero y sus reglas (colisión, detección/colapso de líneas) sin depender de render — verificable por consola de texto, antes de integrar la pieza activa real.

### Story 2.1: Estructuras de estado base y tablero vacío verificable

Como desarrollador,
quiero definir `game_state.h` y un `board.c` con el tablero estático inicializado en ceros,
para tener el contrato de datos entre módulos y una primera pieza lógica verificable sin hardware de video.

**Acceptance Criteria:**

**Given** el proyecto compila con `game_state.h` y `board.c`
**When** se imprime el tablero por la consola de texto de PVSnesLib (`consoleInitText`)
**Then** se ven las 20×10 celdas visibles en cero
**And** ningún módulo fuera de `board.c` escribe directamente el array del tablero

### Story 2.2: Colisión contra bordes y celdas ocupadas

Como desarrollador,
quiero una función de colisión (`board_is_cell_occupied`/chequeo de límites),
para que cualquier movimiento futuro de pieza pueda validarse antes de aplicarse.

**Acceptance Criteria:**

**Given** un tablero con algunas celdas ocupadas de prueba (fijadas por debug, no por juego real)
**When** se consulta la función de colisión en coordenadas dentro/fuera de límites y sobre celdas ocupadas/vacías
**Then** el resultado impreso por consola de texto coincide con el esperado en los 3 casos (límite, ocupada, vacía)

### Story 2.3: Detección y colapso de líneas completas

Como desarrollador,
quiero detectar filas completas del tablero y colapsarlas (shift-down) de inmediato,
para tener la regla de line-clear lista antes de conectarla al ciclo real de la pieza activa.

**Acceptance Criteria:**

**Given** un tablero de prueba con una o más filas completas (rellenadas por debug)
**When** se ejecuta la detección/colapso de líneas
**Then** las filas completas desaparecen y las filas superiores bajan la cantidad correspondiente
**And** el resultado se verifica impreso por consola de texto
**And** el colapso ocurre en el mismo paso, sin delay de animación

---

## Epic 3: Ciclo de vida de la pieza activa (lógica pura)

Implementar spawn, cola de siguiente pieza, movimiento horizontal simple, gravedad y lock — todo verificable por consola de texto antes de dibujar nada.

### Story 3.1: Tabla de formas y spawn de pieza

Como desarrollador,
quiero una tabla estática de formas (rotación 0, adaptada de Apotris) y una función de spawn que coloque una pieza en la posición inicial del tablero,
para tener la primera pieza activa lógica del juego.

**Acceptance Criteria:**

**Given** el tablero vacío de la Story 2.1
**When** se invoca `piece_spawn()`
**Then** aparece una pieza activa en la posición de spawn con la forma correcta de su tipo (rotación 0)
**And** el tipo/posición se verifica impreso por consola de texto

### Story 3.2: Cola de siguiente pieza (7-bag)

Como desarrollador,
quiero una cola de siguiente pieza basada en 7-bag usando `rand()` de PVSnesLib,
para que el spawn use una secuencia justa de piezas sin repetición dentro de cada bolsa de 7.

**Acceptance Criteria:**

**Given** múltiples ciclos de `piece_spawn()` consecutivos
**When** se imprime por consola de texto la secuencia de piezas generadas
**Then** cada grupo de 7 piezas consecutivas contiene cada tipo exactamente una vez
**And** la secuencia cambia entre ejecuciones (no es determinista/fija)

### Story 3.3: Movimiento horizontal simple

Como jugador,
quiero mover la pieza activa una celda a la izquierda o derecha por cada pulsación de pad,
para poder posicionarla en el tablero.

**Acceptance Criteria:**

**Given** una pieza activa spawneada
**When** se pulsa izquierda o derecha (sin auto-repeat / DAS / ARR — un paso por pulsación)
**Then** la pieza se mueve exactamente una celda en esa dirección si no hay colisión
**And** no se mueve si la celda destino está ocupada o fuera del tablero

### Story 3.4: Gravedad automática (Q8.8)

Como desarrollador,
quiero que la pieza activa caiga automáticamente según la tabla de gravedad Q8.8 del nivel actual,
para reproducir el ritmo de caída de Apotris sin usar floats.

**Acceptance Criteria:**

**Given** una pieza activa spawneada y un nivel fijo de prueba
**When** transcurren frames sin input
**Then** la pieza desciende una fila cada N frames según el valor Q8.8 de la tabla de gravedad para ese nivel
**And** el acumulador usa shift+máscara (sin división) para determinar los pasos por frame

### Story 3.5: Lock de la pieza activa

Como desarrollador,
quiero que la pieza activa se fije al tablero cuando ya no puede descender,
para que el tablero refleje las piezas colocadas y pueda generarse la siguiente.

**Acceptance Criteria:**

**Given** una pieza activa que colisiona al intentar descender
**When** se cumple el lock-delay
**Then** la pieza escribe sus celdas en el tablero
**And** se invoca `piece_spawn()` para la siguiente pieza
**And** si la pieza fijada queda por encima del límite superior definido, se marca condición de derrota básica (sin pantalla de game-over, solo el estado lógico)

### Story 3.6: Line clear integrado al ciclo de lock

Como jugador,
quiero que al fijar una pieza se limpien automáticamente las filas que quedaron completas,
para que el tablero refleje la regla estándar de Tetris.

**Acceptance Criteria:**

**Given** un lock que completa una o más filas
**When** se ejecuta la detección/colapso de la Story 2.3 inmediatamente después del lock
**Then** las filas completas se eliminan y el tablero se recoloca antes del siguiente spawn
**And** el ciclo completo (spawn → movimiento → gravedad → lock → line-clear → spawn) es verificable de punta a punta por consola de texto

---

## Epic 4: Integración de render, input y DMA

Conectar la lógica ya validada a pantalla real: sprites para la pieza activa, input real del pad, y DMA del tilemap del playfield.

### Story 4.1: Render de la pieza activa vía sprites OAM

Como jugador,
quiero ver la pieza activa en pantalla mientras cae y se mueve,
para poder jugar visualmente en vez de depender de la consola de texto.

**Acceptance Criteria:**

**Given** el ciclo lógico completo de la pieza activa (Epic 3) ya validado
**When** se inicializan los gráficos de sprites (`oamInitGfxSet`) y se reposicionan con `oamSet`/`oamSetXY` cada frame
**Then** la pieza activa se ve en pantalla en la posición correcta (hasta 4 sprites, una celda ocupada cada uno)
**And** los sprites se ocultan (`oamSetVisible`) inmediatamente después del lock, hasta el siguiente spawn

### Story 4.2: Input real del pad

Como jugador,
quiero controlar la pieza activa con el pad real (SNES o emulador),
para jugar sin depender de ningún input simulado de depuración.

**Acceptance Criteria:**

**Given** `input.c` traduciendo `pad_keys`/`pad_keysdown` (`KEY_LEFT`/`KEY_RIGHT`) a un `InputIntent`
**When** el jugador presiona izquierda/derecha en el pad
**Then** la pieza en pantalla se mueve según la Story 3.3, usando el input real
**And** ningún módulo de gameplay referencia directamente las constantes de PVSnesLib (solo `input.c`)

### Story 4.3: DMA del tilemap del playfield en lock/line-clear

Como jugador,
quiero ver reflejadas en pantalla las piezas fijadas y las líneas eliminadas,
para que el tablero visual coincida siempre con el estado lógico del juego.

**Acceptance Criteria:**

**Given** un buffer WRAM espejo del tilemap del playfield
**When** ocurre un lock o un line-clear (Stories 3.5/3.6)
**Then** el buffer se actualiza y se transfiere completo a VRAM vía `dmaCopyVram` inmediatamente después de `WaitForVBlank()`
**And** el playfield en pantalla refleja exactamente el estado del tablero lógico tras cada lock/line-clear
**And** ninguna transferencia de VRAM ocurre fuera de este evento (no hay DMA por movimiento/gravedad)
