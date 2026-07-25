---
baseline_commit: 7c48ac6
baseline_dirty:
  - path: CLAUDE.md
    md5: 9b491b3f9b61a0a1df1b9ab4f610ec80
  - path: README.md
    md5: 0761a1453e6dd8a9841bd478c9d9c2d3
  - path: _bmad-output/project-context.md
    md5: c5571b8f9844198fdd6f301553f65e78
  - path: pantalla.txt
    md5: 572f2338a7c81869489608fd3beb6376
  - path: snes/piece.png
    md5: 291edda429e5ffd42d891b033da424a6
  - path: _bmad-output/implementation-artifacts/smoke-1-color-pieza-activa.md
    md5: cf3e4ac04fc52e2c139e5bb918b6e728
story_class: herramientas
minimum_validation: V0 + V1
---

# Process 1 — Baseline limpio y V0 reproducible

**Status:** done — cuarta Review aprobada con nits; F1 y F2 aplicados

> **Story de proceso.** No procede de `epics.md` y no toca el juego. Resuelve los
> hallazgos H3 y H4 de la Story de humo
> (`smoke-1-color-pieza-activa.md`), ambos con disposición `defer` y sin destino
> durable.

## Story

Como orquestador del BMAD Game Dev Native Loop,
quiero que el alcance de una Story sea medible aunque el árbol arranque sucio, y
que la Review pueda reproducir V0 de verdad,
para que las dos comprobaciones que sostienen el veredicto —contención de alcance y
verificación independiente— dejen de ser nominales antes de abordar Stories de
gameplay grandes.

## Hallazgos de entrada

**H3 — Baseline declarado sobre árbol sucio.** El baseline es un commit, pero si el
árbol ya tenía cambios antes de empezar la Story, el diff contra ese commit mezcla
el alcance real con la suciedad previa. La Review no puede afirmar *"no se tocó nada
fuera de alcance"*; solo *"veo cosas que no sé a quién atribuir"*. Ya ocurrió tres
veces: al commitear `main.c`, al commitear `CLAUDE.md`, y en la Story de humo.

**H4 — V0 no es reproducible por la Review.** `make` sobre un árbol ya construido
devuelve `No se hace nada para 'all'` con éxito. El revisor firma un no-op creyendo
que reprodujo la compilación. La regla de proyecto de no usar `make clean` cierra la
salida obvia, porque limpiaría la ROM que queda lista para el emulador.

## Clasificación

| Campo | Valor |
|---|---|
| **Clase** | `herramientas` |
| **Nivel mínimo exigido** | V0 + V1 |
| **Derivación** | El diff toca únicamente `tools/`, documentación de proceso y artefactos de Story. No entra código en la ROM, así que no hay dimensión perceptual que auditar |
| **Regla de desempate aplicada** | Ninguna. No hay ambigüedad: cero impacto en el juego |

### Nota sobre la evidencia de esta clase

`herramientas` exige V1, cuyo papel aquí es de **guarda de regresión**: confirma que
las herramientas nuevas no rompieron el harness existente. Pero la evidencia de que
*las herramientas nuevas funcionan* no la produce ni V0 ni V1 — la produce
ejercitarlas. Los AC lo cubren de forma explícita. La brecha de la política queda
registrada como hallazgo diferido, no se resuelve aquí.

## Baseline

`7c48ac6`, con seis archivos ya modificados antes de empezar, declarados en
`baseline_dirty` del frontmatter. Ese conjunto **es la primera aplicación real del
mecanismo que esta Story construye**: la Review debe restarlo del diff antes de
comprobar contención de alcance.

Los cinco primeros son trabajo previo ajeno. El sexto es la Story de humo, que sigue
bloqueada a la espera de su acta de V2 y por tanto sin commitear.

## Allowlist

- `tools/loop/**` (nuevo)
- `BOOTSTRAP.md`
- `docs/BMAD_GAMEDEV_NATIVE_LOOP.md`
- `_bmad-output/implementation-artifacts/process-1-baseline-limpio-y-v0-reproducible.md`

Explícitamente **fuera**: todo `.c`, `.h`, `.png` y el `Makefile`. Ningún artefacto
de la ROM. `docs/BMAD_NATIVE_LOOP.md` no se toca bajo ninguna circunstancia.

## Acceptance Criteria

### H3 — Baseline limpio

1. **AC1** — Existe una herramienta que, ejecutada al crear una Story, emite el
   `baseline_commit` y el conjunto `baseline_dirty` listos para pegar en el
   frontmatter. El conjunto se obtiene del estado real del repositorio, no a mano.
2. **AC2** — Existe una herramienta que, dado un archivo de Story, calcula el
   **conjunto atribuible**: los archivos que difieren del baseline **menos** los
   declarados en `baseline_dirty`. Es lo que la Review debe auditar contra la
   allowlist.
3. **AC3** — Si el árbol está limpio al crear la Story, la herramienta lo indica y
   `baseline_dirty` queda vacío. El camino feliz no obliga a ceremonia extra.
4. **AC4** — La herramienta de comprobación **falla de forma explícita** si un
   archivo declarado en `baseline_dirty` ya no difiere del baseline, porque eso
   significa que la declaración quedó obsoleta y podría estar ocultando un cambio
   real de la Story.

### H4 — V0 reproducible

5. **AC5** — Existe una herramienta que reproduce V0 desde cero **sin tocar el árbol
   de trabajo principal** y **sin usar `make clean`**: construye en una copia
   desechable, partiendo sin artefactos generados.
6. **AC6** — La herramienta compara la ROM que produce con la del árbol principal e
   informa si coinciden. Una ROM que no se puede reproducir desde las fuentes es un
   hallazgo, no un detalle.
7. **AC7** — Ejecutada sobre el estado actual, la reproducción **coincide** con la
   ROM del árbol principal.
8. **AC8** — El árbol principal queda intacto tras ejecutarla: misma ROM, mismos
   artefactos, sin archivos nuevos. Verificado por md5 antes y después.

### Documentación e integración

9. **AC9** — `BOOTSTRAP.md` documenta ambos procedimientos: el gate de árbol sucio en
   su sección de baseline, y la reproducción de V0 en su sección de comandos de
   verificación.
10. **AC10** — `docs/BMAD_GAMEDEV_NATIVE_LOOP.md` actualiza qué debe comprobar la
    Review, para que la re-ejecución independiente de V0 deje de ser nominal.
11. **AC11** — Ningún archivo `.c`, `.h`, `.png` ni el `Makefile` aparecen en el
    conjunto atribuible, y la ROM del árbol es **byte-reproducible desde el contenido
    de `snes/` en el árbol de trabajo**.

    > **Reformulado dos veces.** La redacción original pedía que "la ROM no cambia",
    > y eso **no es verificable de forma independiente**: la ROM está gitignorada, no
    > existe hash pre-Story de origen ajeno al ejecutor, y ninguna herramienta del
    > proyecto puede cerrarlo (R11). Un AC que solo el ejecutor puede afirmar no es
    > un criterio de aceptación.
    >
    > La primera reformulación dijo "desde las fuentes **trackeadas**", y **también
    > afirmaba de más** (T1): `rebuild_v0.py` copia el árbol de trabajo de `snes/`,
    > no el contenido de `HEAD`. La tercera Review lo demostró — la ROM construida
    > desde `HEAD` es `17489e87…`, y la del árbol `25e5dc57…`, porque
    > `snes/piece.png` está modificado sin commitear. La redacción actual afirma
    > exactamente lo que la herramienta comprueba, ni un ápice más.

Todos los AC son verificables al cerrar la Story.

## Tasks / Subtasks

- [x] T1 — Herramienta de snapshot de baseline (AC1, AC3)
- [x] T2 — Herramienta de conjunto atribuible (AC2, AC4)
- [x] T3 — Herramienta de reproducción de V0 (AC5, AC6)
- [x] T4 — Ejercitar las tres herramientas y registrar su salida (AC7, AC8)
- [x] T5 — Actualizar `BOOTSTRAP.md` (AC9)
- [x] T6 — Actualizar la capa de adaptación (AC10)
- [x] T7 — V0 y V1, y comprobar la reproducibilidad de la ROM (AC11)

## Dev Notes

- El procedimiento de reproducción no es una invención: es el que el revisor
  independiente de la Story de humo improvisó por necesidad —copiar `snes/`,
  restaurar el archivo del baseline, compilar aparte— y con el que obtuvo una ROM
  byte-idéntica. Esta Story lo convierte en herramienta documentada en lugar de
  dejarlo a que cada revisor lo redescubra.
- Los artefactos generados a eliminar en la copia son los que `.gitignore` ya
  enumera para `snes/`. Esa lista es la fuente, para que no se dupliquen criterios.
- `PVSNESLIB_HOME` es obligatoria para compilar y hay que propagarla a la copia.
- No usar `make clean` en el árbol principal en ningún momento: la ROM tiene que
  quedar lista para el emulador.
- Riesgo: rutas absolutas en `snes/linkfile`. Se regenera en cada enlace, así que
  no debería importar — pero AC7 lo detectaría si importara.

## Dev Agent Record

### Implementation Plan

Dos herramientas de stdlib bajo `tools/loop/`, una por hallazgo, más la
documentación que las hace parte del procedimiento en vez de scripts sueltos.

Para H3 la clave es que el mecanismo sea **mecánico y no disciplinado**: la lección 5
del framework dice que instruir a un agente a no hacer algo no equivale a impedirlo.
Por eso el conjunto sucio se obtiene del repositorio, no se escribe a mano, y una
declaración obsoleta **bloquea** en lugar de avisar.

Para H4 la clave es no tocar el árbol principal: se compila en una copia desechable
y se compara el binario. Eso reproduce V0 y además comprueba que la ROM que se
validó en el emulador es la que las fuentes producen.

### Debug Log

- `story_baseline.py check` reventaba con `ValueError` en `relative_to` si el archivo
  de Story estaba fuera del repositorio. Salió al probar AC4 con un archivo de
  prueba en el scratchpad. Corregido con fallback a ruta absoluta.
- `rebuild_v0.py` lee los patrones de artefactos generados del `.gitignore` en vez de
  duplicar la lista: si mañana el build produce un artefacto nuevo, se declara en un
  solo sitio. Purgó 26 archivos en la copia.
- V0 en el árbol principal devolvió `No se hace nada para 'all'`. Es correcto —la ROM
  no cambió— y es exactamente el síntoma de H4. La compilación reproducida de verdad
  la dio `rebuild_v0.py`, que recompiló los ocho módulos.

### Completion Notes

H3 y H4 resueltos con herramienta y documentación. Ambos mecanismos se ejercitaron
sobre esta misma Story:

- **H3 se demostró sobre sí mismo.** Esta Story arrancó con seis archivos ya sucios
  —incluida la Story de humo, todavía bloqueada—. Declarados en `baseline_dirty`, el
  conjunto atribuible quedó en exactamente los cinco archivos de la allowlist.
- **H4 se demostró sobre la ROM actual**, que resultó reproducible byte a byte desde
  las fuentes.

Un hallazgo nuevo queda registrado sin implementar, por alcance: la política no tiene
nivel de validación para "la herramienta nueva funciona". Ver más abajo.

## File List

- `tools/loop/story_baseline.py` — nuevo
- `tools/loop/rebuild_v0.py` — nuevo
- `BOOTSTRAP.md` — modificado (§1 reproducción de V0, §6 gate de árbol sucio, §9 índice)
- `docs/BMAD_GAMEDEV_NATIVE_LOOP.md` — modificado (qué verifica la Review)
- `_bmad-output/implementation-artifacts/process-1-baseline-limpio-y-v0-reproducible.md` — nuevo

Sin cambios en ningún `.c`/`.h`/`.png`. La ROM del árbol es byte-reproducible desde el contenido de `snes/` (AC11); su invariancia respecto a un estado previo no observado quedó declarada fuera de alcance por R11.

## Registro de validación

| Nivel | Estado | Evidencia |
|---|---|---|
| V0 | **PASS** | `cd snes && make` → `No se hace nada para 'all'`, exit 0. Correcto: la Story no toca la ROM. Compilación reproducida de verdad vía `rebuild_v0.py` → `PASS`, ROM `25e5dc57…` byte-idéntica |
| V1 | **PASS** | `python3 tools/harness/harness.py` → exit `0`, dominio `WRAM`, 14 lecturas, frames 0→203, cadencia 30 frames. Sin regresión |
| V2 | **no exigida** por la clase `herramientas` | Deuda anotada abajo |

### Evidencia por AC

| AC | Evidencia |
|---|---|
| AC1, AC3 | `story_baseline.py snapshot` emitió el frontmatter con los 6 archivos sucios detectados del repositorio, y avisó por stderr recomendando limpiar primero |
| AC2 | `check` sobre esta Story: 11 difieren del baseline − 6 declarados = **5 atribuibles**, idénticos a la allowlist |
| AC4 | Story de prueba declarando `snes/source/main.c` (que no difiere): `DECLARACION OBSOLETA (1)` y exit `1` |
| AC5, AC6 | `rebuild_v0.py`: copia desechable, 26 artefactos purgados, recompilación completa de los 8 módulos, sin `make clean` en el árbol principal |
| AC7 | ROM reproducida `25e5dc572d06db5a46f7d40e8e8aa79a` = ROM del árbol |
| AC8 | Tras el patch R6, `rebuild_v0.py` compara los 48 archivos de `snes/` antes y después: sin altas, bajas ni cambios |
| AC9 | `BOOTSTRAP.md` §1 "Reproducción independiente de V0", §6 "Gate de árbol sucio", §9 índice |
| AC10 | `docs/BMAD_GAMEDEV_NATIVE_LOOP.md`: "Contención de alcance con árbol sucio" y "Reproducir V0 de verdad" |
| AC11 | Conjunto atribuible sin `.c`/`.h`/`.png`/`Makefile`. `rebuild_v0.py` reproduce la ROM byte a byte desde el contenido de `snes/` |

### Deuda de validación manual

Esta Story se cierra sin V2. La clase `herramientas` no la exige y el diff no puede
alterar la ROM — confirmado por AC11. Anotada aquí a la espera del registro durable,
que sigue sin existir (requisito de adopción nº3).

### Hallazgo nuevo — no implementado por alcance

**H8 — La política no tiene nivel de validación para "la herramienta nueva
funciona".** V0 comprueba que compila; V1 es guarda de regresión sobre una ROM que no
cambió. Ninguno de los dos verifica que `story_baseline.py` o `rebuild_v0.py` hagan
lo que prometen: eso lo cubrieron los AC de esta Story, a mano. Para una Story de
clase `herramientas` eso significa que el nivel mínimo derivado de la clase es
insuficiente por sí solo, y depende de que el Autor escriba buenos AC.
**Disposición: `defer`.** Sin destino durable todavía.

## Review independiente

Subagente con contexto limpio, sin las conclusiones del ejecutor.
**Veredicto: Cambios requeridos.**

El revisor reprodujo todo lo verificable del registro de validación —5 atribuibles,
26 purgados, ROM `25e5dc57…`, V1 con 14 lecturas y frames 0→203— y no encontró
ninguna afirmación falsa. Lo que encontró fue **falta de cobertura**: el ejecutor
probó AC3 solo por el lado de `snapshot`, y AC4 solo por el caso que sí bloquea.

Además ejercitó las herramientas contra casos borde e intentó romperlas
deliberadamente. Ahí salieron los defectos reales.

### Hallazgos y disposición

| # | Hallazgo | Disposición | Estado |
|---|---|---|---|
| R1 | `snapshot` emite `baseline_dirty: []` con árbol limpio y `check` lo iteraba como caracteres, bloqueando con exit 1. **Rompía AC3**: las dos mitades de la herramienta no se hablaban | `patch` | **resuelto** |
| R2 | `check` resta **por ruta, no por contenido**: un archivo ya declarado sucio puede modificarse más o borrarse durante la Story sin detección | `decision-needed` → decidido: implementar hash por ruta | **resuelto** |
| R3 | La declaración no está atada a un instante: re-ejecutar `snapshot` a mitad de Story blanquea cambios fuera de alcance | `decision-needed` → decidido junto con R2 | **mitigado, no cerrado** |
| R4 | Parseo de renames incorrecto en modo `-z`: producía rutas fantasma y dejaba la ruta vieja en el conjunto atribuible | `patch` | **resuelto** |
| R5 | El purgado estaba acoplado en silencio a `.gitignore`. Con las líneas de `.obj` ausentes, la herramienta dio **PASS a una ROM no reproducible** | `patch` | **resuelto** |
| R6 | `BOOTSTRAP.md` afirmaba verificar el árbol "por md5 antes y después"; verificaba **un** archivo | `patch` | **resuelto** |
| R7 | La capa de adaptación prometía un bloqueo más fuerte del implementado | `patch` | **resuelto** |
| R8 | Entradas inválidas revientan con traceback. Falla cerrado, pero ilegible | `defer` | pendiente |
| R9 | Listas YAML con guion en columna 0 se ignoraban en silencio | `defer` → resuelto de paso al arreglar R1 | **resuelto** |
| R10 | `tools/loop/` no estaba en la tabla de convenciones de rutas de `BOOTSTRAP.md` §2 | `patch` | **resuelto** |
| R11 | La mitad "la ROM no cambia" de AC11 no es verificable de forma independiente: no hay ROM versionada ni hash pre-Story de origen ajeno al ejecutor | `decision-needed` | **abierto** |
| R12 | Confirma H8: ni V0 ni V1 tocaron las herramientas nuevas. El nivel mínimo de la clase `herramientas` no cubre su propio objeto | `defer` | pendiente |

### Verificación de los patches

Cada corrección se comprobó con la misma prueba que la tumbó:

| Patch | Prueba | Resultado |
|---|---|---|
| R1 | Round-trip `snapshot` (árbol limpio) → `check` | `0 declarado(s)`, `OK`, exit 0 |
| R9 | Frontmatter con guion en columna 0 | `2 declarado(s)` — antes 0 |
| R4 | Repo de prueba con rename staged + modificación | `['a.txt', 'otro.txt', 'renamed_a.txt']` — ruta vieja y nueva, sin fantasma |
| R5 | `.obj` superviviente tras purgar | Aborta: `el purgado dejo artefactos que harian falsa la reproduccion (1)` |
| R5b | Árbol ya purgado | No aborta |
| R6 | `rebuild_v0.py` completo | `arbol principal intacto .. si (48 archivos comparados)` |
| Regresión | AC4 y la Story real | exit 1 y exit 0 respectivamente, sin cambios |

Verificación tras cada patch, no en bloque, según la regla del framework.

### Decisión del orquestador sobre R2/R3: hash por ruta declarada

`baseline_dirty` pasa de lista de rutas a lista de `path` + `md5` del contenido en
el momento de declararlo. `check` resta una ruta **solo si su contenido sigue siendo
el declarado**.

| Estado | Comportamiento |
|---|---|
| Sigue difiriendo, md5 igual | Se resta |
| Sigue difiriendo, md5 distinto | **No se resta** — entra en el conjunto atribuible |
| Borrada durante la Story | No se resta — entra, con md5 `(borrado)` |
| Ya no difiere del baseline | **FAIL**, exit 1 (comportamiento previo, conservado) |
| Sin `md5` (formato heredado) | Se resta por ruta, con aviso de punto ciego |

Ambos ataques del revisor, reproducidos contra la versión nueva:

| Ataque | Antes | Ahora |
|---|---|---|
| Modificar `pantalla.txt`, declarado sucio | Silencioso, `OK`, exit 0 | Entra en el atribuible: `pantalla.txt (572f2338 -> 4917f6af)` y `5 restado(s)` de 6 |
| Borrar `pantalla.txt` | Silencioso, `OK`, exit 0 | Entra en el atribuible con `(borrado)` |
| Restaurar el archivo | — | Vuelve a `6 restado(s)`, `OK`, exit 0 |

**R3 queda mitigado, no cerrado, y así está documentado.** Regenerar el snapshot a
mitad de Story sigue blanqueando: los hashes se actualizan al estado nuevo. La
diferencia es que ya no es silencioso — exige reescribir a mano cada valor, y es un
cambio visible y deliberado en el archivo de Story. Cerrarlo del todo exigiría
commitear el frontmatter en `Create Story`, antes de que empiece la ejecución. Eso
cambia el flujo de trabajo y queda como trabajo diferido.

### Re-verificación tras la decisión

| Comprobación | Resultado |
|---|---|
| `check` sobre esta Story | `6 declarado(s), 6 restado(s)`, atribuible = 5 = allowlist, `OK` |
| V0 | `No se hace nada para 'all'` — correcto, la Story no toca la ROM |
| `rebuild_v0.py` | `PASS`, ROM `25e5dc57…`, 48 archivos comparados |
| V1 | `PASS` |
| `pantalla.txt` tras las pruebas destructivas | md5 `572f2338…`, idéntico al declarado |

### Nota de aislamiento

Idéntica a la de la Story de humo: contexto realmente separado, pero mismo proceso
anfitrión y mismo modelo, el prompt del ejecutor es un canal de encuadre, y la
prohibición de escribir fue por instrucción y no por allowlist estructural.

En esta Story el aislamiento **rindió de forma medible**: el revisor tumbó AC3 y
encontró dos vías de blanqueo que el ejecutor no había considerado, precisamente
porque no compartía sus supuestos.

## Re-Review (segunda ronda)

**Veredicto: Cambios requeridos.** Nueve de diez correcciones se sostuvieron, pero
el revisor encontró que mi patch de R4 arregló **solo una mitad** de la herramienta,
y que el mecanismo de contenido tenía cuatro escapes y un punto ciego estructural.

Reprodujo cada número declarado —5 atribuibles, 26 purgados, 48 comparados, 8
módulos, 14 lecturas, frames 0→203, ROM `25e5dc57…`— sin encontrar ninguna
afirmación falsa. Amplió AC8 verificando **435 archivos** de todo el repositorio.

### Hallazgos de la segunda ronda

| # | Hallazgo | Disposición | Estado |
|---|---|---|---|
| N2 | **Asimetría de renames.** Arreglé `dirty_paths()` pero no `changed_vs()`, que usaba `git diff` con detección de renames. Producía a la vez un falso FAIL (REG-1) y un **bypass de AC11** (REG-2): `git mv` de un `.c` fuera de alcance a un directorio permitido lo hacía desaparecer del conjunto atribuible mientras el `.c` se destruía | `patch` | **resuelto** |
| N3 | **Escape del mecanismo de contenido.** `md5` ausente, o con clave no parseable (`md5 :`, `MD5:`, tabuladores), volvía a restar por ruta con aviso solo por stderr y **exit 0**. Cuatro formas de anular la comparación sin que nada bloqueara | `patch` | **resuelto** |
| N1 | **Evasión por `.gitignore`.** Todo archivo ignorado es invisible al conjunto atribuible. Sin preparación alguna, el `.gitignore` actual ya oculta `build/`, `out/`, `dist/`, `*.o` — y un `.gitignore` anidado que se ignora a sí mismo oculta un directorio entero | `patch` | **resuelto** |
| N4 | `check` imprimía `OK … es coherente` y devolvía 0 tras haber impreso `DECLARADOS PERO MODIFICADOS` | `patch` | **resuelto** |
| N5 | `check` no valida `baseline_commit`: sin comprobación de ancestría ni de identidad con el HEAD de `Create Story` | `defer` | pendiente |
| N6, N7 | Investigados y descartados como explotables por el propio revisor | `Rejected by Design` | cerrados |
| R11 | La ROM está gitignorada, así que **no puede** aparecer nunca en el conjunto atribuible y su invariancia no es verificable por nadie salvo el ejecutor | `decision-needed` → decidido: aceptar el límite | **resuelto por reformulación de AC11** |

### Verificación de la segunda ronda

| Patch | Prueba | Resultado |
|---|---|---|
| N2 / REG-2 | Repo sintético: `git mv src/main.c allowed/moved_main.c` | `changed_vs` devuelve **ambas** rutas: `['allowed/moved_main.c', 'src/main.c', 'st/s.md']`. Antes solo la destino |
| N2 / REG-1 | Round-trip `snapshot` → `check` con rename staged | El par se resta correctamente (`2 restado(s)` de 3). Sin falso `DECLARACION OBSOLETA` |
| N3 | `md5 :` con espacio de más | `DECLARADOS SIN md5 (1)`, exit **1** |
| N3b | `md5:` omitido | `DECLARADOS SIN md5 (1)`, exit **1** |
| N1 | `tools/loop/build/helper.py` (dir ignorado por `.gitignore`) | `IGNORADOS NO ESPERADOS (1)`, exit **1** |
| N4 | Declarado y modificado | exit **1**, sin `OK` final |
| Regresión | Estado normal, declaración obsoleta, `baseline_dirty` vacío | exit 0 / 1 / 0 — los tres correctos |

Al implementar N1 salieron dos ignorados legítimos que faltaban por declarar:
`_bmad/custom/config.user.toml` (configuración local de BMAD) y
`tools/lua/poc_read_memory.log` (log del harness). Declarados como esperados.

### Verificación final

`check` exit 0 con atribuible = 5 = allowlist · `rebuild_v0.py` exit 0 ·
V1 exit 0 · ROM `25e5dc57…` y `pantalla.txt` `572f2338…` intactos.

### Nota del revisor sobre el aislamiento

Declaró algo que conviene conservar: el scratchpad contenía artefactos de la sesión
de Review anterior (`renametest`, `pantalla.bak`, copias de repo), y los leyó. **Es
un canal de información entre revisores que el aislamiento nominal no contempla.**
Lo reportó en lugar de disimularlo.

## Tercera Review (acotada a los patches)

**Veredicto: Cambios requeridos.** N1–N4 se sostuvieron, cero regresiones, y el
camino feliz siguió en exit 0. Cuatro bloqueantes nuevos, todos aplicados.

| # | Hallazgo | Disposición | Estado |
|---|---|---|---|
| T1 | **AC11 seguía afirmando de más.** Escribí "reproducible desde las fuentes **trackeadas**", pero `rebuild_v0.py` copia el árbol de trabajo. Demostrado: ROM desde `HEAD` = `17489e87…` ≠ `25e5dc57…`, porque `snes/piece.png` está modificado sin commitear. La reformulación reintrodujo el defecto que debía eliminar | `patch` | **resuelto** |
| T2 | **`assume-unchanged` / `skip-worktree`** sacan un trackeado modificado de `git diff` y `git status`, y por tanto del conjunto atribuible. Misma clase de invisibilidad que N1 y N2 | `patch` | **resuelto** |
| T3 | Afirmación falsa en tres sitios: que los tabuladores no se reconocen en la clave `md5:`. Sí se reconocen; el mensaje de error dirigía a una causa inexistente | `patch` | **resuelto** |
| T4 | Cuatro asertos obsoletos en esta Story contradiciendo el AC11 reformulado | `patch` | **resuelto** |
| T5 | `EXPECTED_IGNORED_SUFFIXES` valía en todo el repositorio, no solo bajo `snes/`. Cinco vectores demostrados | `patch` | **resuelto parcialmente** — sufijos acotados a `snes/`; los directorios de material no-código quedan como puntos ciegos declarados |
| T6 | Repo git anidado colapsa a una entrada de directorio; degrada la auditoría por extensión | `defer` | pendiente |
| T7 | `".ps"` era código muerto en la lista de sufijos | `patch` | **resuelto** |
| T8 | Paréntesis desbalanceado al truncar `(borrado)` con `[:8]` | `patch` | **resuelto** |

### Verificación de la tercera ronda

| Patch | Prueba | Resultado |
|---|---|---|
| T2 | Repo sintético: `git update-index --assume-unchanged src/main.c` + modificarlo | `git status` no lo ve; `check` → `OCULTOS A GIT STATUS (1): src/main.c [h]`, exit **1**. Al quitar la marca, el `.c` aparece en el atribuible |
| T5 | `docs/hidden.map` (sufijo de build fuera de `snes/`) | `IGNORADOS NO ESPERADOS (1)`, exit **1** |
| T5b | `snes/source/zz_tmp.obj` (artefacto legítimo) | exit **0**, sin ruido |
| Regresión | Estado normal de la Story | atribuible = 5 = allowlist, exit **0** |
| Final | `check` / `rebuild_v0.py` / V1 | exit 0 los tres. ROM `25e5dc57…`, `pantalla.txt` `572f2338…`, `piece.png` `291edda4…`, `CLAUDE.md` `9b491b3f…` intactos |

### Nota del revisor sobre higiene

Además de la fuga de scratchpad ya declarada en la ronda 2, señaló que
`tools/loop/__pycache__/*.pyc` quedó dentro del repositorio como residuo de la
ronda anterior: está ignorado y declarado como esperado, así que es invisible a
`check`. Es un ejemplo concreto del punto ciego que T5 deja abierto a propósito.

## Cuarta Review (acotada a los patches T1–T8)

**Veredicto: Aprobado con nits.** Los ocho patches se sostienen. Cero regresiones.
Ningún bloqueante nuevo.

Modo: revisión inline por decisión del orquestador (sin subagente aislado), por
presupuesto de tokens. **Esto degrada la ronda respecto a las tres anteriores**: el
revisor comparte el contexto del ejecutor. Registrado como limitación, no como
equivalencia.

### Verificación independiente re-ejecutada

| Comprobación | Resultado |
|---|---|
| `story_baseline.py check` | exit 0 · 6 declarados, 6 restados, 11 difieren · atribuible = 5 = allowlist exacta |
| V0 (`make`) | exit 0, `No se hace nada para 'all'` · ROM `25e5dc57…` |
| `rebuild_v0.py` | exit 0 · 26 purgados · 48 archivos comparados, árbol intacto · ROM reproducida `25e5dc57…` = árbol |
| V1 (`harness.py`) | exit 0 · `WRAM`, 14 lecturas, frames 0→203 |
| Estado final | `apotris.sfc` `25e5dc57…`, `pantalla.txt` `572f2338…`, `piece.png` `291edda4…`, índice sin marcas (`ls-files -v` todo `H`) |

Cada número del registro de validación se reprodujo. **Ninguna afirmación falsa.**

### Patches re-verificados

| Patch | Comprobación | Resultado |
|---|---|---|
| T1 | AC11 vs. lo que `rebuild_v0.py` hace realmente (copia el árbol de trabajo, no `HEAD`) | La redacción afirma exactamente eso. Sin residuos de "fuentes trackeadas" fuera del relato histórico |
| T2 | Ataque en vivo sobre el repo real: `git update-index --assume-unchanged snes/source/main.c` | `OCULTOS A GIT STATUS (1): snes/source/main.c [h]`, exit 1. Marca revertida; índice limpio |
| T3 | Las tres afirmaciones sobre tabuladores contra el parser (`stripped = line.lstrip()`) | Correctas. `md5 :` y `MD5:` no se reconocen; la sangría con tabulador sí |
| T5 | `EXPECTED_IGNORED_BUILD` exige prefijo `snes/`; `build/`, `out/`, `dist/` siguen entrando en `IGNORADOS NO ESPERADOS` | Acotado como se declaró. El sufijo `.log` global solo alcanza a `tools/lua/*.log`, que es el único `.log` que Git ignora |
| T7 | `".ps"` ausente de la lista | Confirmado muerto: el `Makefile` borra los `.ps` al terminar el enlace |
| T8 | Truncado de `(borrado)` en el lado *actual* | Correcto — pero ver F1 |

### Hallazgos y disposición

| # | Hallazgo | Disposición |
|---|---|---|
| F1 | T8 se arregló **en un solo lado**. `story_baseline.py:308` protege `current` con `if current == DELETED`, pero `declared_md5[:8]` no. Un archivo declarado con `md5: (borrado)` que reaparezca imprime `((borrado -> 572f2338)` — mismo paréntesis desbalanceado que T8 eliminaba. Reproducido | `patch` |
| F2 | `tools/loop/__pycache__/*.pyc` sigue en el árbol. Ignorado y declarado como esperado, luego invisible a `check`. Es el punto ciego de T5 materializado dentro del propio entregable | `patch` |
| F3 | Durante un merge sin resolver, `git ls-files -v` marca los archivos con `M`. `hidden_from_status()` los trata como ocultos y emite el mensaje de `assume-unchanged`, que dirige a una causa inexistente. Falla cerrado, pero diagnostica mal | `defer` |

Descartados tras comprobarlos: `.o` ausente de `MUST_NOT_SURVIVE` (el toolchain emite
`.obj`, no `.o`); sufijo `.log` con alcance global (solo alcanza rutas que Git ya
ignora); rutas con caracteres especiales en `--name-only` sin `-z` (no existen en este
repositorio); `purge_generated` sin patrones de directorio (`build/`, `out/` y `dist/`
no existen bajo `snes/`, y `MUST_NOT_SURVIVE` los cubriría por `**/*.obj`).

### Review Findings

- [x] [Review][Patch] Truncado asimétrico de `(borrado)` en el lado declarado [tools/loop/story_baseline.py:308] — aplicado
- [x] [Review][Patch] Eliminar `tools/loop/__pycache__/` del árbol [tools/loop/] — aplicado
- [x] [Review][Defer] `hidden_from_status()` confunde archivos sin fusionar (`M`) con `assume-unchanged` [tools/loop/story_baseline.py:143] — diferido, falla cerrado

> Los `defer` se anotan aquí porque el destino durable sigue sin existir
> (requisito de adopción nº3). No se creó `deferred-work.md`: definirlo es decisión
> del orquestador, no de la Review.

### Aplicación de F1 y F2, y cierre

Aplicados los dos `patch` de la cuarta Review, y nada más.

- **F1** — `story_baseline.py`: el lado declarado del mensaje pasa por la misma guarda
  que el actual. `(borrado)` ya no se trunca a 8 caracteres en ninguno de los dos
  lados.
- **F2** — `tools/loop/__pycache__/` borrado del árbol. Ejecutar los scripts
  directamente no lo regenera: Python solo cachea módulos importados.

**Por qué no procede una quinta Review completa.** El único cambio de código es una
guarda de formato en la rama de error de `check`, sin efecto sobre lo que se resta,
lo que entra en el conjunto atribuible ni el código de salida. F2 borra un artefacto
ignorado. Ninguno de los dos puede alcanzar la ROM, así que V0 y V1 —ejecutados y en
PASS en esta misma sesión— siguen siendo válidos. Se re-ejecutó lo que sí podía
romperse:

| Prueba | Resultado |
|---|---|
| F1 — declarado `(borrado)`, archivo presente | `pantalla.txt  ((borrado) -> 572f2338)` — paréntesis equilibrado |
| Regresión — declaración obsoleta | exit **1** |
| Regresión — `baseline_dirty: []` | `0 declarado(s), 0 restado(s)`, exit 0 |
| Regresión — Story real | atribuible = 5 = allowlist, exit **0** |
| Árbol | sin `__pycache__`, sin temporales, sin ocultos a `git status`, sin ignorados no esperados. ROM `25e5dc57…` |

### Estado final de los hallazgos

| Ronda | Resueltos | Abiertos |
|---|---|---|
| 1 (R1–R12) | R1–R7, R9, R10, R11 (por reformulación de AC11) | R8, R12 → `defer` |
| 2 (N1–N7) | N1–N4, N6/N7 rechazados por diseño | N5 → `defer` |
| 3 (T1–T8) | T1–T4, T5 (parcial), T7, T8 | T6 → `defer` |
| 4 (F1–F3) | F1, F2 | F3 → `defer` |

**Cero bloqueantes. Cero `decision-needed` abiertos.**

`defer` pendientes de destino durable: R8 (tracebacks en entrada inválida), R12/H8
(la política no valida que la herramienta nueva funcione), N5 (`baseline_commit` sin
comprobación de ancestría), T6 (repo git anidado), F3 (`M` de merge confundido con
`assume-unchanged`), y el punto ciego declarado de T5 (directorios de material
no-código).

**Status: done.**

## Change Log

| Fecha | Cambio |
|---|---|
| 2026-07-25 | Story creada (Create Story) |
| 2026-07-25 | Implementación (T1–T7): dos herramientas en `tools/loop/`, documentación en `BOOTSTRAP.md` y en la capa de adaptación. V0 y V1 en PASS |
| 2026-07-25 | Review independiente: Cambios requeridos. 12 hallazgos; 7 resueltos por `patch`, 3 `decision-needed` bloquean el cierre, 2 `defer` |
| 2026-07-25 | Decisión del orquestador sobre R2/R3: hash por ruta declarada. Implementado |
| 2026-07-25 | Re-Review: Cambios requeridos. 4 bloqueantes nuevos (N1–N4), todos resueltos por `patch`. R11 y N5 siguen abiertos |
| 2026-07-25 | Decisión del orquestador sobre R11: aceptar el límite y reformular AC11 |
| 2026-07-25 | Tercera Review (acotada): Cambios requeridos. 4 bloqueantes (T1–T4) más T5/T7/T8, todos aplicados. T6 y N5 `defer` |
| 2026-07-25 | Cuarta Review (acotada, inline): Aprobado con nits. T1–T8 se sostienen, cero regresiones. 2 `patch` cosméticos (F1, F2), 1 `defer` (F3) |
| 2026-07-25 | F1 y F2 aplicados y re-verificados. Story cerrada en `done` |
