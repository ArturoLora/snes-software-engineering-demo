---
baseline_commit: 1149bfb
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
story_class: integracion
minimum_validation: V0 + V1 + V2
review_level: B
---

# Story 4.9 — Line clear integrado al ciclo de lock

**Status:** in-progress — V0, V1 y Review B en PASS. **A la espera del acta de V2**

> Corresponde a **Story 3.6 de `epics.md`**, nunca ejecutada. Va como `4-9` porque
> pertenece al ciclo del Epic 4 y `3-8` ya está tomada — ver `BOOTSTRAP.md` §3.
> **Con esta Story se completa el alcance mínimo declarado en `project-context.md`.**

## Story

Como jugador,
quiero que al fijar una pieza desaparezcan las filas que quedaron completas y el
resto baje,
para que el tablero se pueda vaciar y la partida no se muera en un minuto.

## Estado de partida

`board_detect_full_lines()` y `board_collapse_lines()` existen y están probadas desde
la Story 2.3, y **siguen sin conectarse a nada**. El ciclo lock → tablero → render →
spawn quedó cerrado en la Story 4.8. Falta un único eslabón: detectar y colapsar
entre el lock y el bake del tablero.

Sin él la pila solo crece, y el hallazgo G2 de la 4.8 llega en menos de un minuto: la
pila alcanza la zona de spawn y la simulación deja de avanzar.

## Clasificación

| Campo | Valor |
|---|---|
| **Clase** | `integracion` — conecta dos sistemas existentes y cambia el gameplay de forma sustantiva |
| **Nivel mínimo** | V0 + V1 + **V2** |
| **Nivel de Review** | **B** |
| **Derivación de la Review** | `integracion` da B por tabla. **Ningún disparador de escalado aplica**: no añade estado en WRAM —reutiliza `gs.lines`, que existe desde la Story 2.1—, no toca DMA ni VRAM —`render_sync_board()` se llama, no se modifica—, no toca `tools/` ni el `Makefile` |

## Allowlist

- `snes/source/main.c`
- `_bmad-output/implementation-artifacts/4-9-line-clear-integrado.md`

Fuera: **todo lo demás**. En particular `board.c` —sus primitivas ya sirven tal
cual—, `render.*`, `piece.*`, `queue.*`, `game_state.h` y todos los assets. Esta
Story conecta lo que ya existe; si hiciera falta modificar una primitiva, la clase y
el alcance estarían mal.

## Acceptance Criteria

1. **AC1** — Tras cada lock se detectan las filas completas y, si hay alguna, se
   colapsan **antes** de que aparezca la pieza siguiente.
2. **AC2** — El colapso ocurre **antes** del bake del tablero, de modo que el DMA de
   ese mismo frame ya muestra el tablero recolocado. Nunca se ve un estado intermedio
   con la fila completa dibujada.
3. **AC3** — Las filas por encima de una eliminada **bajan**; las de debajo no se
   mueven.
4. **AC4** — Un lock que no completa ninguna fila no cambia el tablero más allá de
   las celdas de la propia pieza. Sin colapsos espurios.
5. **AC5** — El número de filas eliminadas en el último lock es **visible por
   consola**, que es la evidencia de punta a punta que pide el AC del roadmap.
6. **AC6** — No se añade estado nuevo en WRAM. Se reutiliza `gs.lines`.
7. **AC7** — V1 sigue en `PASS` y la cadencia de gravedad sigue en 30 frames.
8. **AC8** — El conjunto atribuible es exactamente `main.c` y este archivo.
9. **AC9** — En el emulador: al completar una fila, esa fila **desaparece**, lo de
   encima **baja**, y la partida continúa. Con line clear el tablero se puede vaciar.

## Tasks / Subtasks

- [x] **T1 — Conectar el line clear al ciclo de lock** (AC1, AC2, AC3, AC4, AC6)
  - En el bloque de lock del bucle, entre `piece_lock()` y `render_sync_board()`:
    detectar filas completas y colapsarlas si las hay.
  - El orden importa y es la mitad del valor de la Story: `piece_lock` → detectar →
    colapsar → `render_sync_board` → `piece_spawn`.
  - Sin funciones nuevas, sin estado nuevo, sin tocar `board.c`.
- [x] **T2 — Evidencia por consola** (AC5)
  - Imprimir el número de filas detectadas **antes** de colapsar: `board_collapse_lines()`
    pone el contador a cero, así que después ya no hay nada que leer.
  - Fila 25 de la consola, que está libre.
- [x] **T3 — V0** (AC8)
- [x] **T4 — V1** (AC7)
- [x] **T5 — Contención de alcance y contrato de direcciones**
- [ ] **T6 — V2 en Ares, con acta** (AC9)

## Dev Notes

- **`board_collapse_lines()` pone `gs.lines.count = 0` al terminar.** Cualquier lectura
  del número de filas eliminadas tiene que ocurrir antes de llamarlo. Es la única
  trampa de esta Story.
- El bake del tablero ya es genérico: `render_sync_board()` recorre `board[][]` entero
  y reconstruye el espejo. No necesita saber que hubo un colapso.
- **G2 no se cierra aquí.** El line clear hace posible vaciar el tablero, pero un
  jugador que apile mal seguirá llegando a la zona de spawn, y ahí el comportamiento
  sigue siendo el registrado en la 4.8. El top-out sigue sin pertenecer a ninguna
  Story.
- No entra scoring, ni combo, ni animación de borrado, ni contador acumulado de líneas
  — todo eso es alcance de Stories que no existen.

## Dev Agent Record

### Completion Notes

17 líneas en un archivo. El line clear entra entre `piece_lock()` y
`render_sync_board()`, y el orden es la mitad del valor: colapsar antes del bake
significa que el espejo del tilemap nunca llega a contener la fila completa. El
contador se lee antes de colapsar, porque `board_collapse_lines()` lo pone a cero.

Sin estado nuevo, sin funciones nuevas, sin tocar `board.c`. Las primitivas de la
Story 2.3 se usan tal cual, tres años de distancia narrativa después de escribirse.

## Review independiente — nivel B

Primera Review bajo la política v1.3 con alcance acotado. **Veredicto: Aprobado.
Cero hallazgos** de severidad alta, media o baja.

El revisor re-ejecutó V0, V1 y la contención, y auditó los 9 AC contra el diff y el
código modificado, sin salirse de la superficie permitida. **Coste: ~42k tokens**,
frente a los ~92k y ~105k de las dos Reviews de nivel A de esta misma sesión.

Comprobación que conviene destacar, porque era el riesgo real de la Story: **colapsar
varias filas en un mismo lock**. `board_detect_full_lines()` recorre `y` ascendente,
así que `rows[]` queda ordenado, y colapsar en ese orden no invalida los índices
posteriores —un índice mayor que la fila colapsada no se desplaza—. El revisor lo
verificó a mano para `{18,19}` adyacentes y `{15,19}` no adyacentes.

### Nit informativo, no accionable

**N1** — AC2 dice "el DMA de ese mismo frame". En realidad `render_flush_board()`
está al tope del bucle, así que el bake del frame N se transfiere al inicio del N+1.
Es la arquitectura que fijó la Story 4.8, no algo que introduzca esta Story, y la
propiedad sustantiva que AC2 exige —que nunca se vea la fila completa dibujada— sí se
cumple, porque el espejo jamás llega a contenerla. Anotado por si el acta de V2 se
redacta con la expectativa literal de "mismo frame".

### Defer registrados por el revisor

- `board_collapse_lines()` no puede eliminar la fila 0 (`j > 0`): si se llenara,
  quedaría permanentemente llena. Asunción documentada de "buffer superior siempre
  vacío", no alcanzable hoy.
- `board_detect_full_lines()` recorta a `LINES_TO_CLEAR_MAX` sin señalarlo.
  Irrelevante mientras un lock no pueda completar más de 4 filas.

## File List

- `snes/source/main.c` — modificado (line clear entre lock y bake, línea `CLR`)
- `_bmad-output/implementation-artifacts/4-9-line-clear-integrado.md` — nuevo

## Registro de validación

| Nivel | Estado | Evidencia |
|---|---|---|
| V0 | **PASS** | `Build finished successfully !`. ROM `89dfcf60…`. `rebuild_v0.py` del revisor: reproducible byte a byte, 51 archivos, árbol intacto |
| V1 | **PASS** | exit `0`, `WRAM`, 13 lecturas, frames 0→186. Cadencia de gravedad **30/30/30/30** exactos |
| V2 | **pendiente** | Exigida por la clase `integracion`. **El Sprint se detiene aquí** |

## Change Log

| Fecha | Cambio |
|---|---|
| 2026-07-25 | Story creada (Create Story, Sprint autónomo) |
| 2026-07-25 | Implementación T1–T5. V0 y V1 en PASS, atribuible = 2 = allowlist |
| 2026-07-25 | Review nivel B: **Aprobado**, cero hallazgos. A la espera de V2 |
