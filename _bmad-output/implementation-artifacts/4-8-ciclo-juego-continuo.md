---
baseline_commit: c08dbb3
baseline_dirty:
  - path: CLAUDE.md
    md5: ae9688e71a4720474f2fae66086d56e5
  - path: README.md
    md5: 0761a1453e6dd8a9841bd478c9d9c2d3
  - path: _bmad-output/implementation-artifacts/process-1-baseline-limpio-y-v0-reproducible.md
    md5: 46c33acc709849c0136f000502a2bfed
  - path: _bmad-output/project-context.md
    md5: c5571b8f9844198fdd6f301553f65e78
  - path: pantalla.txt
    md5: 572f2338a7c81869489608fd3beb6376
  - path: tools/loop/story_baseline.py
    md5: 5e04519372f9d4f72bfef70efbae73b9
story_class: integracion
minimum_validation: V0 + V1 + V2
---

# Story 4.8 — Ciclo de juego continuo: lock, tablero persistente y siguiente pieza

**Status:** done — V0, V1 y V2 en PASS. Los doce AC verificados

> Corresponde a **Story 4.3 de `epics.md`** (DMA del tilemap del playfield), la
> última Story del plan. El archivo se numera `4-8` porque `4-3` ya está ocupado por
> `4-3-gravedad-automatica.md` — ver `BOOTSTRAP.md` §3, "Numeración".

## Story

Como jugador,
quiero que la pieza se fije al aterrizar, quede visible en el tablero y aparezca una
pieza nueva,
para que el juego tenga un ciclo continuo en vez de una sola pieza que se detiene y
no vuelve a pasar nada.

## Estado de partida

El bucle interactivo de hoy hace tres cosas: spawnea **una** pieza, la mueve con el
pad, y la baja un paso cada 30 frames. Cuando toca el suelo, se para. Fin.

Las piezas que faltan **ya existen y están probadas**, solo que desconectadas:

| Función | Dónde | Estado |
|---|---|---|
| `piece_lock()` | `piece.c` | Escribe las 4 celdas de la forma en `board[][]`. **Solo se llama en las pruebas de arranque**, nunca desde el bucle |
| `piece_spawn()` | `piece.c` | Pone `type=0`, `x=3`, `y=2`. Se llama una vez antes del bucle |
| `piece_shape_collides()` | `piece.c`, `static` | Colisión de la forma completa contra el tablero |
| Render del tablero | — | **No existe.** `board[][]` es invisible: nada transfiere el tilemap a VRAM |

Por eso una pieza fijada desaparece de la pantalla: los sprites se ocultan y no queda
rastro. Esta Story cierra el ciclo.

## Clasificación

| Campo | Valor |
|---|---|
| **Clase** | `integracion` |
| **Nivel mínimo exigido** | V0 + V1 + V2 |
| **Derivación** | Conecta lógica ya validada con presentación real: escribe VRAM por DMA y cambia lo que se ve en pantalla en cada lock. La dimensión perceptual es el objeto de la Story, no un efecto colateral |
| **Regla de desempate aplicada** | Ninguna. Toca render y toca el bucle de juego: `integracion` domina sobre `logica-interna` sin ambigüedad |

## Baseline

`c08dbb3`, con seis archivos ya modificados antes de empezar, declarados en
`baseline_dirty`. Ninguno pertenece a esta Story. La Review los resta antes de
auditar la contención de alcance:

```bash
python3 tools/loop/story_baseline.py check \
  _bmad-output/implementation-artifacts/4-8-ciclo-juego-continuo.md
```

## Allowlist

- `snes/source/render.c`, `snes/source/render.h`
- `snes/source/piece.c`, `snes/source/piece.h`
- `snes/source/main.c`
- `snes/playfield.png`
- `_bmad-output/implementation-artifacts/4-8-ciclo-juego-continuo.md`
- `tools/lua/poc_read_memory.lua` — **añadido durante la ejecución** por decisión del
  orquestador, para desbloquear V1. Ver G1

> **Ampliación de allowlist, 2026-07-25.** La allowlist original excluía todo
> `tools/`. Se amplió con un único archivo, y solo después de que el hallazgo G1
> demostrara que sin ese cambio **V1 no puede producir evidencia**. Queda anotado
> aquí, y no borrado, porque una allowlist que se ensancha en silencio no contiene
> nada.

Explícitamente **fuera**: `board.c`/`board.h`, `game_state.h`, `input.c`/`input.h`,
`piece_data.c`/`piece_data.h`, `test_status.*`, `test_runner.*`, el `Makefile`,
`snes/piece.png`, y todo `tools/` y `docs/`.

**`board.c` no se toca.** Sus primitivas bastan: `board_get()` para leer y
`board_init()` para limpiar. Que `board_detect_full_lines()` y
`board_collapse_lines()` existan y sigan sin conectarse es correcto — el line clear
está fuera de alcance.

## Acceptance Criteria

### Ciclo de juego

1. **AC1** — Cuando la gravedad no puede bajar la pieza porque colisiona, la pieza
   se fija: `piece_lock()` escribe sus cuatro celdas en `board[][]`.
2. **AC2** — Inmediatamente después del lock aparece una pieza nueva en la posición
   de spawn, y responde al pad y a la gravedad igual que la anterior.
3. **AC3** — El ciclo se repite indefinidamente: cada pieza que aterriza se fija y da
   paso a la siguiente, sin intervención ni reinicio.
4. **AC4** — Las celdas escritas por locks anteriores **siguen ocupadas**: una pieza
   nueva colisiona contra la pila y se apoya encima, no la atraviesa.

### Render del tablero

5. **AC5** — Existe un buffer espejo del tilemap del playfield en WRAM, inicializado
   desde el mapa generado en ROM. `render.c` es el **único** módulo que lo declara y
   lo escribe.
6. **AC6** — Al hacer lock, el buffer se actualiza para que cada celda ocupada de
   `board[][]` use el tile de celda fijada, y cada celda vacía conserve el tile
   original del asset.
7. **AC7** — El buffer se transfiere completo a VRAM con `dmaCopyVram`,
   **inmediatamente después de `WaitForVBlank()`**.
8. **AC8** — **Ninguna transferencia de VRAM ocurre fuera de ese evento.** Ni el
   movimiento horizontal ni la gravedad disparan DMA — solo mueven sprites.
9. **AC9** — El índice del tile de celda fijada **no se escribe como constante
   literal**: se obtiene leyendo el mapa generado en una coordenada conocida, para
   que un recuento de tiles distinto de `gfx4snes` no lo invalide en silencio.

### Contención

10. **AC10** — `board.c`, `board.h`, `game_state.h`, `input.c`, `piece_data.c` y el
    `Makefile` **no aparecen** en el conjunto atribuible.
11. **AC11** — Ningún **módulo de gameplay** incluye headers de vídeo de PVSnesLib ni
    escribe VRAM/OAM: `piece.c` sigue sin conocer pantalla, y el render del tablero
    vive entero en `render.c`.

    > **Reformulado (Review H4).** La redacción original decía "ningún módulo salvo
    > `render.c`", y eso es **falso** desde la Story 1.1: `main.c` incluye `<snes.h>`
    > e inicializa los backgrounds y la consola de texto. Es preexistente al baseline
    > y esta Story no lo empeora, pero un AC que afirma de más no es un criterio de
    > aceptación. Lo que esta Story sí garantiza, y es lo que el AC mide ahora, es que
    > la lógica de juego no aprendió qué es una pantalla.
12. **AC12** — En el emulador: una pieza cae, al aterrizar **queda dibujada en el
    tablero**, aparece una pieza nueva, y el ciclo continúa. Las piezas se apilan
    visiblemente unas sobre otras.

Todos los AC son verificables al cerrar la Story. Ninguno depende de trabajo futuro.

## Tasks / Subtasks

- [x] **T1 — Tile de celda fijada en el asset** (AC6, AC9)
  - Regenerar `snes/playfield.png` de 256×224px (32×28 tiles) a **256×256px
    (32×32 tiles)**. Las filas 0-27 se conservan **idénticas** al arte actual
    (Stories 4.4/4.5/4.7: relleno + borde de acento en columnas 0-9 filas 3-22,
    relleno plano en el resto).
  - En las filas 28-31 —**fuera del área visible de 28 filas**, así que nunca se
    dibujan— pintar al menos una celda de 8×8 en **color de acento sólido**
    `(96,104,128)`, el mismo del borde. Ese es el tile de celda fijada.
  - El resto de las filas 28-31, relleno plano.
  - Verificar en el log de `gfx4snes` que el mapa generado pasa a `32x32 tiles`.

- [x] **T2 — Buffer espejo y DMA en `render.c`** (AC5, AC6, AC7, AC9, AC11)
  - Declarar el buffer espejo `static` en `render.c`, dimensionado al mapa generado
    (32×32 entradas de 16 bits = 2048 bytes).
  - `render_board_init(void)`: copia el mapa de ROM (`playfieldmap`) al buffer, y
    captura la **entrada de tile de celda fijada** leyendo el buffer en la coordenada
    de la celda sólida que T1 pintó en las filas 28-31. Sin literales de índice.
  - `render_sync_board(GameState *gs)`: recorre las celdas del tablero y escribe en
    el buffer, por cada una, la entrada de celda fijada si `board_get()` la reporta
    ocupada, o la entrada original del mapa de ROM si está vacía. Marca el buffer
    como pendiente de transferir.
  - `render_flush_board(void)`: si hay transferencia pendiente,
    `dmaCopyVram(buffer, 0x7000, tamaño)` y limpia la marca. No-op en caso contrario.
  - Conversión de coordenadas: celda de tablero `(x, y)` → entrada de nametable
    `(columna x, fila y + 1)`. Es el **mismo `+1`** que `render_sync_piece()` ya
    aplica: −2 por las filas de buffer superior del tablero, +3 por el
    desplazamiento de centrado de la Story 4.7. Reutilizar el razonamiento, no
    reinventarlo.

- [x] **T3 — Detección de aterrizaje en `piece.c`** (AC1, AC11)
  - Añadir `piece_is_landed(GameState *gs)`: devuelve verdadero si la pieza no puede
    bajar un paso más. Reutiliza el `piece_shape_collides()` que ya existe.
  - No duplicar la comprobación en `main.c`: es una regla de juego, y `main.c` no
    contiene reglas de juego (`game-architecture.md` §2).

- [x] **T4 — Cerrar el ciclo en el bucle de `main.c`** (AC1, AC2, AC3, AC4, AC7, AC8)
  - Al principio del bucle, es decir justo después del `WaitForVBlank()` de la
    iteración anterior: `render_flush_board()`.
  - Dentro del bloque de tick de gravedad ya existente, tras
    `piece_apply_gravity()`: si `piece_is_landed()`, entonces `piece_lock()`,
    `render_sync_board()` y `piece_spawn()`, en ese orden.
  - Llamar `render_board_init()` una vez en el arranque, y `render_sync_board()` una
    vez tras el `board_init()` previo al bucle, para que el estado inicial del
    tablero y el del buffer coincidan.
  - No añadir DMA en ninguna otra ruta.

- [x] **T5 — V0** (AC10)
  - `cd snes && make`. Sin `make clean`.
  - Confirmar en el log de `gfx4snes` que `playfield.map` se regeneró, y que
    `data.obj` y la ROM cambiaron — la regla de assets de `CLAUDE.md` exige
    comprobar la cadena, no confiar en que `make` terminó bien.

- [x] **T6 — V1** (guarda de regresión)
  - `python3 tools/harness/harness.py`. Debe seguir en `PASS`.
  - La cadencia de gravedad observada debe seguir siendo de 30 frames: esta Story no
    toca la velocidad de caída.

- [x] **T7 — V2 en Ares, con acta** (AC12)
  - Observar y registrar: la pieza cae, se fija al aterrizar, **queda visible**,
    aparece una nueva, y las piezas se apilan.

- [x] **T8 — Contención de alcance** (AC10, AC11)
  - `python3 tools/loop/story_baseline.py check <esta Story>` → el conjunto
    atribuible debe ser exactamente los archivos de la allowlist (8 tras la
    ampliación por G1).
  - `python3 tools/loop/rebuild_v0.py` → la ROM debe seguir siendo reproducible.

## Dev Notes

### Por qué el buffer espejo, y no escritura de celda directa

Ya está investigado y confirmado, **no repetir la investigación**:
`_bmad-output/implementation-artifacts/investigations/pvsneslib-tilemap-runtime-update-investigation.md`.
`background.h` no expone escritura de celda individual — ninguna de sus 12 funciones
lo hace. El patrón real, tomado del ejemplo oficial `games/breakout/breakout.c` (que
es equivalente en escala: romper un ladrillo), es mantener el espejo en WRAM,
mutarlo, y transferirlo con `dmaCopyVram` tras `WaitForVBlank()`.

`game-architecture.md` §8 ya decidió transferir el **tilemap completo** en cada
evento en lugar de trackear regiones sucias. A este tamaño, trackear regiones sería
complejidad sin beneficio medible. El presupuesto de la arquitectura hablaba de
~400 bytes cuando el mapa era 10×20; hoy, con el canvas de 32 tiles de ancho de la
Story 4.5 y el centrado de la 4.7, la transferencia real son 2048 bytes. Sigue siendo
la cuarta parte de lo que breakout transfiere por evento equivalente, y ocurre solo
en el lock.

### Por qué el tile sólido va en las filas 28-31

El área visible del SNES son 28 filas; el nametable `SC_32x32` tiene 32. Las cuatro
últimas existen en VRAM y **nunca se dibujan**. Poner ahí la celda de acento sólido
mete el tile en el tileset sin que aparezca en pantalla.

Esto además hace que el índice del tile sea **descubrible en tiempo de ejecución**:
se lee del propio mapa generado, en la coordenada donde T1 lo pintó. Si mañana
`gfx4snes` deduplica distinto y renumera los tiles, el código sigue funcionando. Un
`#define LOCKED_TILE 7` se rompería en silencio, y el síntoma sería un tablero
dibujado con el tile equivocado — exactamente la clase de fallo que V0 no detecta.

Por el mismo motivo el buffer se inicializa **copiando el mapa de ROM**, no
sintetizando el fondo: el arte de las Stories 4.4/4.5/4.7 se conserva sin que este
código sepa nada de él.

### Riesgos identificados

- **2048 bytes de WRAM en un `static`.** Es la primera estructura grande del
  proyecto. Si el enlazado protesta por el tamaño de sección, es un hallazgo
  legítimo — no reducirlo transfiriendo por trozos sin registrarlo primero.
- **`dmaCopyVram` y las unidades de dirección.** `0x7000` es el valor que
  `bgInitMapSet` ya recibe en `main.c`. Si la transferencia aterriza desplazada, el
  síntoma será un tablero corrido: comprobar si la función espera dirección en
  palabras o en bytes antes de tocar cualquier otra cosa.
- **Rutas absolutas y orden de arranque.** `render_board_init()` tiene que correr
  después de `bgInitMapSet` y antes de `setScreenOn()`, igual que `render_init()`.

### Fuera de alcance — declarado

No implementar, aunque el código quede a un paso: **line clear** (las primitivas
existen en `board.c` y siguen desconectadas a propósito), **7-bag** (`piece_spawn()`
sigue dando siempre `type=0`), **gravedad Q8.8**, **lock delay**, **puntuación**,
**niveles**, **HUD**, **audio** y **rotación**.

**Top-out no se maneja.** Sin line clear, la pila crece hasta la zona de spawn y la
pieza nueva acabará naciendo dentro de celdas ocupadas. La Story no define qué debe
pasar entonces, porque el top-out no está en el alcance de ninguna Story todavía.
Registrar el comportamiento observado como hallazgo diferido en lugar de inventar una
regla.

Si durante la implementación aparece trabajo adicional, **registrarlo como hallazgo
o trabajo diferido**, no implementarlo, salvo que sin él la Story no pueda cerrarse.

### Reglas de proyecto que aplican aquí

- Nunca `make clean` en el árbol principal: la ROM tiene que quedar lista para Ares.
- Memoria estática, sin allocación dinámica.
- `render.c` es el único módulo que incluye headers de vídeo y el único que escribe
  VRAM/OAM (`game-architecture.md` §7). `piece.c` no puede aprender qué es una
  pantalla.
- Antes de escribir un algoritmo nuevo, mirar `reference/apotris`. Para esta Story,
  `piece_lock()` ya adaptó `Game::place()`; lo que falta es integración, no
  algoritmia.
- Sin Internet. Si un problema resulta ser de PVSnesLib o del toolchain y dos
  hipótesis locales ya se descartaron, detenerse y proponer `/gds-investigate`.

## Dev Agent Record

### Implementation Plan

Tres piezas pequeñas y una de asset. El ciclo se cierra en el bucle, no en un módulo
nuevo: `piece.c` aporta la regla (`piece_is_landed`), `render.c` aporta la
presentación (espejo + DMA), y `main.c` solo secuencia lock → bake → spawn.

### Debug Log

- El tile de celda fijada quedó en el índice **9** del tileset: el mapa generado tiene
  ahora 1024 entradas (2048 bytes, antes 1792), con `0x9` en la fila 28 columna 0,
  frente a `0x0` del relleno interior y `0x4` del borde izquierdo. Los tres distintos,
  que es lo que AC9 pedía verificar sin literales.
- `render_board_init()` copia byte a byte usando el tamaño real
  `&playfieldmap_end - &playfieldmap`, acotado al máximo del buffer. Si el mapa
  generado creciera, no desborda.
- El riesgo de WRAM anotado en las Dev Notes no se materializó: el enlazado reporta
  RAM slot 0 con 25129 bytes libres (76,69%). Pero el buffer **sí desplazó el layout
  de WRAM**, y eso destapó G1 (ver abajo).

### Completion Notes

Las cuatro tareas de implementación están hechas y V0 pasa. El ciclo de juego, medido
sobre WRAM real, funciona: la pieza spawnea en `x=3 y=2` y la gravedad la baja
`y=3→4→5→6→7` en los frames 66/96/126/156/186 — cadencia de 30 frames, sin regresión.

**Lo que bloquea el cierre no es el juego: es el instrumento que lo mide.** Ver G1.

## Hallazgo G1 — V1 lee direcciones fijas y da PASS en falso

**Severidad: bloqueante.** Es para V1 lo que H4 era para V0.

`tools/lua/poc_read_memory.lua` tiene las direcciones **hardcodeadas**:

```lua
local BUS_PAD0 = 0x7E2000
local BUS_GS   = 0x7E2002
```

El buffer de 2048 bytes que introduce esta Story se asignó en `$7E2000` y empujó todo
lo demás hacia arriba:

| Símbolo | Antes | Ahora |
|---|---|---|
| `board_map` (nuevo) | — | `$7E2000` |
| `pad0` | `$7E2000` | `$7E2805` |
| `gs` | `$7E2002` | `$7E2807` |

Así que el Lua pasó a leer **mi buffer de tilemap** creyendo que era `GameState`. El
log lo dice sin decirlo: `piece[type=0 rot=0 x=0 y=0]` congelado durante toda la
corrida — son entradas de relleno del tilemap, no una pieza.

**Y el harness devolvió `PASS`.** Sus tres criterios —≥2 lecturas, ≥180 frames
avanzados, "estado de juego visto"— se satisfacen igual de bien leyendo basura. La
única señal fue que las cifras cambiaron respecto a corridas anteriores: 6 lecturas y
186 frames, frente a 14 y 203. Reproducido tres veces para descartar jitter.

**Verificado que el juego está bien.** Apuntando el Lua temporalmente a `$7E2805` y
`$7E2807`, el log muestra el comportamiento correcto (spawn `x=3 y=2`, gravedad
`y=3→4→5→6→7` cada 30 frames). El Lua se restauró a su contenido original,
comprobado por md5 `708bf083…` — el diagnóstico no dejó rastro.

**Disposición: `decision-needed` → decidido por el orquestador: ampliar la allowlist
y aplicar el parche mínimo.**

Las dos constantes pasan a `$7E2805`/`$7E2807`, y la cabecera del script gana una
advertencia explícita: que son constantes pegadas a mano, que cualquier Story que
añada una variable en WRAM las invalida, que cuando eso ocurre **el harness sigue
devolviendo PASS**, y cuál es el primer comando a ejecutar ante un log incoherente.

Con eso V1 vuelve a medir el juego, y **esta Story cierra con evidencia real en vez
de con un PASS hueco**.

**Lo que el parche NO cierra.** El arreglo correcto es resolver ambos símbolos desde
`apotris.sym` en tiempo de ejecución — los dos existen ahí con nombre (`pad0`,
`tccs_source/main.asm_gs`). Eso es un cambio de diseño del harness, no un parche, y
excede una Story de gameplay. **G1 queda abierto** como candidato a Story de clase
`herramientas`, con la misma forma que tuvo `process-1` para H3/H4.

Hasta entonces el defecto sigue vivo: la próxima Story que añada una variable en WRAM
volverá a romperlo. La diferencia es que ya no es invisible — está documentado en el
propio archivo que falla, con el comando de diagnóstico al lado.

## File List

- `snes/playfield.png` — modificado (256×224 → 256×256; filas 0-223 byte-idénticas,
  celda de acento sólido añadida en la fila 28 del nametable)
- `snes/source/render.c` — modificado (espejo del tilemap, `render_board_init`,
  `render_sync_board`, `render_flush_board`)
- `snes/source/render.h` — modificado (las tres funciones nuevas)
- `snes/source/piece.c` — modificado (`piece_is_landed`)
- `snes/source/piece.h` — modificado (`piece_is_landed`)
- `snes/source/main.c` — modificado (arranque del espejo, flush al inicio del bucle,
  lock → bake → spawn en el tick de gravedad)
- `_bmad-output/implementation-artifacts/4-8-ciclo-juego-continuo.md` — nuevo

Artefactos regenerados por `make`, no versionados: `playfield.pic`/`.map`/`.pal`,
`data.obj`, `apotris.sfc`, `apotris.sym`.

## Registro de validación

| Nivel | Estado | Evidencia |
|---|---|---|
| V0 | **PASS** | `cd snes && make` → `Build finished successfully !`, exit 0. Cadena de assets comprobada por md5, no por ausencia de errores: `playfield.map` 1792 → **2048 bytes** (32×32 tiles), `data.obj` → `53c5ef71…`, ROM `25e5dc57…` → `adb60ea1…` → **`1b4bb990…`** tras los patches de Review. Reproducibilidad: `rebuild_v0.py` → `PASS`, ROM `1b4bb990…` byte-idéntica, 48 archivos comparados, árbol intacto |
| V1 | **PASS** | Tras el parche de G1: exit `0`, dominio `WRAM`, **15 lecturas**, frames 0→186. El log muestra el juego real — spawn `x=3 y=2`, gravedad `y=3→4→5→6→7` en los frames 66/96/126/156/186. **Cadencia de 30 frames sin regresión**, que es lo que V1 vigila aquí. La corrida previa al parche (6 lecturas, valores congelados en 0) queda registrada en G1 como lo que era: un PASS hueco |
| V2 | **PASS** | Acta firmada por Arturo el 2026-07-25 sobre la ROM `1b4bb990…`. Ver "Acta de V2" abajo |

### Contención de alcance

`story_baseline.py check` → conjunto atribuible = **8 archivos, idénticos a la
allowlist ampliada**. `board.c`, `board.h`, `game_state.h`, `input.c`, `piece_data.c`,
`test_*` y el `Makefile` no aparecen (AC10). Ningún módulo salvo `render.c` incluye
headers de vídeo (AC11).

## Review independiente

Subagente con contexto limpio, sin las conclusiones del ejecutor y sin permiso de
escritura sobre el árbol. **Veredicto: Aprobado con nits.** Ocho hallazgos, todos de
severidad `menor`. Cero bloqueantes.

Reprodujo V0, V1 y la contención **cifra por cifra**, y no encontró ninguna
afirmación falsa sobre el estado actual. Además hizo algo que la Story no pedía:
montó una **sonda Lua propia** en su scratchpad y la lanzó por el harness durante
~1600 frames, leyendo a la vez el array `board[][]` en WRAM y **el nametable en el
dominio VRAM de BizHawk**. Con eso verificó AC1–AC8 de forma empírica en lugar de por
lectura de código:

| Observación de la sonda | Qué demuestra |
|---|---|
| `boardOcc` 0 → 4 → 8 → 12 | AC1: el lock escribe cuatro celdas |
| Tras cada lock, `piece(x=3,y=2)` y reanuda descenso a 30 frames | AC2 |
| Tres ciclos completos sin intervención en f≈600 / 1110 / 1590 | AC3 |
| Filas 19, 20 y 21 ocupadas en columnas 3-6, cada pieza una fila más arriba | AC4: la pila sostiene |
| Tablero filas 19/20/21 → nametable filas 20/21/22, mismas columnas | AC6: la aritmética `+1` es correcta contra VRAM real |
| Conteo de tiles `9` en VRAM: 0 → 4 → 8 → 12, un salto por lock y ninguno entre medias, pese a 17 ticks de gravedad | AC8: no hay DMA fuera del lock |

### Hallazgos y disposición

| # | Hallazgo | Severidad | Disposición |
|---|---|---|---|
| H1 | La primerísima DMA corría **antes** del primer `WaitForVBlank()`, con la pantalla ya encendida. Inocuo hoy solo por accidente —con el tablero vacío el bake reproduce los bytes del propio asset—, pero en cuanto el estado inicial no sea vacío la primera imagen dependería de una transferencia inválida | `menor` | `patch` — **resuelto** |
| H2 | `render_board_init()` acota el tamaño del mapa por arriba pero no por abajo. Si el mapa volviera a 32×28, `locked_entry` quedaría en 0 y las celdas fijadas se dibujarían **invisibles**, sin error de compilación | `menor` | `patch` de la **afirmación**, no del código — ver abajo |
| H3 | El registro decía «7 archivos» y T8 decía «6». El valor real es **8** | `menor` | `patch` — **resuelto** |
| H4 | AC11 afirmaba que ningún módulo salvo `render.c` incluye headers de vídeo. Falso: `main.c` incluye `<snes.h>` y escribe VRAM desde la Story 1.1 | `menor` | `patch` — **resuelto** |
| H5 | El marco de acento vive **dentro** de las celdas jugables, así que las piezas del suelo y de las paredes lo borran. Agravante: el tile de celda fijada usa el mismo color que el marco | `menor` | `decision-needed` → decidido en el acta de V2: **aceptable**, mejora visual futura |
| H6 | El comentario que justificaba la decisión de diseño describía un «tick de descanso» que no existe | `menor` | `patch` — **resuelto** |
| H7 | El parche de G1 añade **documentación, no detección**. El Lua sigue sin comprobar plausibilidad y el harness sigue dando PASS con basura | `menor` | `defer` → insumo para la Story de G1 |
| H8 | V1 no ejercita AC1–AC4: el harness cierra a los 186 frames y el primer lock ocurre hacia el 600 | `menor` | `defer` — cobertura, no defecto |

### Correcciones aplicadas

- **H1** — el bake inicial se transfiere ahora **antes de `setScreenOn()`**, con la
  pantalla en forced blank, donde las escrituras a VRAM sí valen. De paso limpia el
  flag, así que el primer `render_flush_board()` del bucle es el no-op que debía ser.
- **H6** — comentario corregido: el lock ocurre en el **mismo** tick en que la pieza
  alcanza su fila final. La traza del revisor lo prueba —`f=570 y=19`, `f=600 y=2`,
  y `y=20` no se observa nunca—. El único caso que espera un tick es el de una pieza
  que queda apoyada por un movimiento horizontal entre dos ticks.
- **H3, H4** — cifras y redacción corregidas en este archivo.

**Sobre H2 no se toca el código.** El revisor explícitamente no pidió implementarlo, y
tiene razón: el fallo solo se dispara si alguien revierte el asset de T1, cosa que la
propia T1 verifica. Lo que sí era incorrecto era **la afirmación**: AC9 protege contra
la *renumeración* de tiles, no contra el *cambio de tamaño* del mapa. Registrado aquí
en lugar de dejar la Story reclamando una robustez que solo cubre la mitad del espacio
de fallo.

### Re-validación tras los patches

| Comprobación | Resultado |
|---|---|
| V0 | `Build finished successfully !`, exit 0. ROM `adb60ea1…` → **`1b4bb990…`** |
| `rebuild_v0.py` | `PASS`, ROM `1b4bb990…` byte-idéntica, árbol intacto |
| V1 | `PASS`, 15 lecturas, frames 0→186, cadencia de 30 frames |
| Contención | Atribuible = 8 = allowlist, exacto |

### Lo que el revisor intentó romper y aguantó

Desbordamiento de `u8` en los índices (se ensancha a `u16` antes de multiplicar,
índice máximo real 713) · `map_bytes` sin inicializar · cota superior del tamaño del
mapa · orden de inicialización · unidades de dirección de `dmaCopyVram` (confirmado
contra `dma.h` y empíricamente) · solape de rango en VRAM (`0x7000`+1024 words no pisa
ni el mapa de texto en `0x6800` ni los tiles en `0x4000`) · lectura de ROM a través de
`u16 *` · pérdida o exceso de disparos del flag · coherencia del primer frame ·
reinicio del contador de gravedad tras el lock.

### Nota de aislamiento

Contexto realmente separado y prohibición de escritura respetada: `git status` al
terminar era idéntico al del inicio. Aislamiento **parcial** en el mismo sentido que
las Stories anteriores: mismo proceso anfitrión, mismo modelo, y la prohibición de
escribir fue por instrucción y no por allowlist estructural.

Rindió de forma medible: H1 y H5 no los había considerado el ejecutor, y H3/H4/H6 son
afirmaciones mías que no se sostenían.

## Acta de V2

**Ejecutada por:** Arturo (orquestador humano). **Fecha:** 2026-07-25.
**Emulador:** Ares. **ROM:** `snes/apotris.sfc`, md5 `1b4bb9908a7dbad1e2c2cfd132cb7518`
— la ROM posterior a los patches de Review, y la que se commitea. Reproducible byte a
byte desde el contenido de `snes/` según `rebuild_v0.py`.

**Veredicto: AC12 PASS.** Observado:

| # | Condición | Resultado |
|---|---|---|
| 1 | La pieza cae | **sí** |
| 2 | Al hacer lock permanece en el tablero | **sí** |
| 3 | Se genera automáticamente la siguiente pieza | **sí** |
| 4 | El ciclo continúa | **sí** |
| 5 | Las piezas se acumulan de forma persistente | **sí** |

**Los doce AC quedan verificados.** Es el primer ciclo de juego completo de Apotris
SNES, y el primero desarrollado íntegramente bajo el Game Dev Native Loop.

### Disposición de H5

**Aceptable para esta Story.** El borrado del marco de acento por las piezas del suelo
y de las paredes no bloquea el objetivo funcional. Se deja como mejora visual futura
si se considera necesaria. No se abre trabajo por ello.

## Hallazgo diferido

**G2 — Top-out no manejado. Comportamiento confirmado en V2.** Cuando la pila alcanza
la zona de spawn, **el juego deja de avanzar**: no aparecen piezas nuevas y la
simulación se detiene visualmente.

Coincide con lo que predice el código: la pieza nueva nace dentro de celdas ocupadas,
`piece_apply_gravity()` no puede moverla, `piece_is_landed()` da verdadero de
inmediato, y el bucle entra en lock → spawn → lock sobre las mismas celdas, tick tras
tick. No es un cuelgue —el bucle sigue corriendo y el pad sigue leyéndose— pero desde
fuera es indistinguible de uno.

Anticipado en las Dev Notes antes de implementar, y confirmado como inevitable en este
alcance: sin line clear no hay forma de vaciar el tablero. **Disposición: `defer`** —
insumo para la futura Story de Top-Out / Game Over. El top-out no pertenece a ninguna
Story todavía.

## Change Log

| Fecha | Cambio |
|---|---|
| 2026-07-25 | Story creada (Create Story) |
| 2026-07-25 | Implementación (T1–T5, T8). V0 en PASS, cadena de assets y reproducibilidad de ROM verificadas. Conjunto atribuible = allowlist |
| 2026-07-25 | Hallazgo G1: V1 devolvía PASS leyendo direcciones obsoletas. Decisión del orquestador: ampliar allowlist y aplicar parche mínimo. V1 en PASS con evidencia real |
| 2026-07-25 | Review independiente: Aprobado con nits. 8 hallazgos `menor`; H1/H3/H4/H6 resueltos por `patch`, H5 `decision-needed` para el acta de V2, H2/H7/H8 `defer`. ROM → `1b4bb990…` |
| 2026-07-25 | V2 firmada por el orquestador humano sobre `1b4bb990…`: AC12 PASS. H5 aceptado, G2 confirmado y diferido. Story cerrada en `done` |
