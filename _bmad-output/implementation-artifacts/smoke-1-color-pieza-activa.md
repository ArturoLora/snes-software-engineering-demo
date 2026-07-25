---
baseline_commit: 7c48ac6
story_class: render
minimum_validation: V0 + V1 + V2
---

# Smoke 1 — Color del tile de la pieza activa

**Status:** done — V2 firmada por el orquestador humano; H1 y H2 cerrados

> **Story de humo.** No procede de `epics.md` y no avanza el gameplay. Su objetivo es
> recorrer el Game Dev Native Loop de punta a punta con el menor riesgo técnico
> posible, para validar el *proceso* antes de abordar Stories reales.

## Story

Como desarrollador,
quiero cambiar el color del tile con el que se dibuja la pieza activa,
para verificar que el Game Dev Native Loop funciona de punta a punta en este
proyecto — clasificación, validación por niveles, evidencia y Review — sobre un
cambio cuyo riesgo de romper el juego es prácticamente nulo.

## Clasificación

| Campo | Valor |
|---|---|
| **Clase** | `render` |
| **Nivel mínimo exigido** | V0 + V1 + V2 |
| **Derivación** | El diff altera un asset gráfico y el resultado es perceptible en pantalla. Por la tabla de `docs/BMAD_GAMEDEV_NATIVE_LOOP.md`, `render` exige V2 |
| **Regla de desempate aplicada** | Ninguna. No hay ambigüedad: el cambio es exclusivamente visual |

## Baseline

`7c48ac6` — declarado en el frontmatter. La Review mide el diff contra ese hash,
no contra `HEAD`.

Hashes del árbol en el baseline, para las comprobaciones de la cadena de assets:

| Archivo | md5 en baseline |
|---|---|
| `snes/piece.png` | `7e020282ab3249001f85dc065ae66982` |
| `snes/piece.pal` | `36ed1b0f6bbe3e43e15df8538727ea53` |
| `snes/piece.pic` | `a3d10e328bccb32e6be244106159c144` |
| `snes/data.obj` | `1286590438869f686876cb98d8eff916` |
| `snes/apotris.sfc` | `17489e87f64a381bcfa6ae6d64d0adb4` |

## Allowlist

Rutas que el ejecutor puede modificar. Cualquier archivo fuera de esta lista en el
diff contra el baseline es un hallazgo de Review.

- `snes/piece.png`
- `_bmad-output/implementation-artifacts/smoke-1-color-pieza-activa.md` (solo las secciones que el workflow permite)

Explícitamente **fuera** de la allowlist: todo `.c` y `.h`, el `Makefile`, cualquier
otro asset, y los artefactos generados (que cambian por `make`, no por edición).

## Acceptance Criteria

1. **AC1** — La entrada 1 de la paleta de `snes/piece.png` deja de ser cian
   `(0,255,255)` y pasa a ámbar `(255,160,0)`. El resto del archivo no cambia:
   sigue siendo PNG indexado de 8×8, y todos sus píxeles siguen con índice 1.
2. **AC2** — Tras `make`, el asset generado `snes/piece.pal` **cambia** respecto a
   su md5 de baseline.
3. **AC3** — Tras `make`, `snes/data.obj` **se regenera** (md5 distinto al de
   baseline). Verifica que la dependencia declarada por la Story 4.6 sigue
   funcionando.
4. **AC4** — Tras `make`, `snes/apotris.sfc` **cambia** respecto a su md5 de
   baseline. La ROM no puede quedar idéntica.
5. **AC5** — V1 sigue en `PASS`, y la cadencia de gravedad observada sigue siendo
   de un paso cada 30 frames. Un cambio de paleta no puede alterar la lógica; este
   AC es la guarda de regresión.
6. **AC6** — El diff contra el baseline no toca ningún `.c` ni `.h`.
7. **AC7** — En el emulador, la pieza activa se dibuja en ámbar en lugar de cian, y
   sigue cayendo y moviéndose con el pad exactamente igual que antes.

Todos los AC son verificables al cerrar la Story. Ninguno depende de trabajo futuro.

## Tasks / Subtasks

- [x] T1 — Cambiar la entrada 1 de la paleta de `snes/piece.png` a `(255,160,0)`,
      preservando modo indexado, tamaño 8×8 e índices de píxel (AC1, AC6)
- [x] T2 — V0: ejecutar `make` en `snes/` (AC2, AC3, AC4)
- [x] T3 — Comprobar la cadena de assets: `piece.pal` → `data.obj` → `apotris.sfc`,
      comparando md5 contra el baseline (AC2, AC3, AC4)
- [x] T4 — V1: ejecutar el harness y registrar veredicto y cadencia de gravedad (AC5)
- [x] T5 — V2: validación manual en Ares, con acta (AC7)

## Dev Notes

- `snes/piece.png` es el tile placeholder de 8×8 de la Story 4.1. Todos sus píxeles
  usan el índice 1; el índice 0 es el color transparente del sprite. Solo hay que
  tocar la entrada 1 de la paleta — **no** los datos de píxel.
- La conversión la hace `gfx4snes` desde el `Makefile` con `-s 8 -o 16 -u 16 -e 0 -p -i`.
  El SNES usa 5 bits por canal, así que el ámbar quedará cuantizado. Eso es esperado
  y no invalida AC1, que se comprueba sobre el PNG fuente.
- **No usar `make clean`** (regla de proyecto). Solo `make`.
- Riesgo identificado: que la librería de imagen reescriba el PNG cambiando la
  profundidad de bits o el tamaño de la paleta y `gfx4snes` lo interprete distinto.
  Mitigación: T1 verifica modo, tamaño e índices tras guardar, y AC2/AC3/AC4
  detectarían cualquier ruptura de la cadena.

## Dev Agent Record

### Implementation Plan

Editar únicamente la entrada 1 de la paleta del PNG indexado, sin tocar datos de
píxel, y dejar que `make` propague el cambio por la cadena de assets. Verificar la
propagación por md5 en cada eslabón en lugar de confiar en que `make` terminó bien
— la regla de assets de `CLAUDE.md` es explícita en que terminar sin errores no
demuestra que se regeneró nada.

### Debug Log

- El PNG se reescribió con la librería de imagen preservando modo `P`, tamaño 8×8,
  entrada 0 en `(0,0,0)` y los 64 píxeles en índice 1. Verificado releyendo el
  archivo tras guardarlo.
- `snes/piece.pic` **no cambió**, y es correcto: los índices de píxel del tile son
  los mismos; solo cambió la paleta, que vive en `snes/piece.pal`. Sirve de
  comprobación cruzada de que se tocó lo que se pretendía y nada más.

### Completion Notes

Cambio de un solo valor de paleta, sin código. La cadena de assets se propagó
completa y quedó verificada por md5 en los cuatro eslabones. V1 confirma que la
lógica no se movió: misma cadencia de gravedad de 30 frames que en el baseline.

Hallazgo de proceso, no del cambio: el diff contra el baseline arrastra cuatro
archivos modificados **antes** de que esta Story empezara (`CLAUDE.md`,
`README.md`, `_bmad-output/project-context.md`, `pantalla.txt`). No los tocó esta
Story. Ver la retrospectiva.

## File List

- `snes/piece.png` — modificado (entrada 1 de la paleta)
- `_bmad-output/implementation-artifacts/smoke-1-color-pieza-activa.md` — nuevo

Artefactos regenerados por `make`, no versionados: `snes/piece.pal`,
`snes/data.obj`, `snes/apotris.sfc`, `snes/apotris.sym`.

## Registro de validación

| Nivel | Estado | Evidencia |
|---|---|---|
| V0 | **PASS** | `cd snes && make` → `Build finished successfully !` |
| V1 | **PASS** | `python3 tools/harness/harness.py` → exit `0`, dominio `WRAM`, 14 lecturas, frames 0→203, cadencia de gravedad 30 frames (f=53,83,113,143,173,203) |
| V2 | **PASS** | Acta firmada por Arturo el 2026-07-25 sobre la ROM `25e5dc57…`. Ver "Acta de V2" abajo |

### Cadena de assets (AC2 / AC3 / AC4)

| Archivo | Baseline | Tras `make` | Cambió |
|---|---|---|---|
| `snes/piece.png` | `7e020282…` | `291edda4…` | sí |
| `snes/piece.pal` | `36ed1b0f…` | `95511174…` | sí |
| `snes/piece.pic` | `a3d10e32…` | `a3d10e32…` | no — esperado |
| `snes/data.obj` | `12865904…` | `dd4e0089…` | sí |
| `snes/apotris.sfc` | `17489e87…` | `25e5dc57…` | sí |

## Review independiente

Ejecutada por un subagente con contexto limpio, autorizado explícitamente por el
orquestador humano. **Veredicto: Bloqueado.**

El revisor no recibió las conclusiones del ejecutor, solo punteros a artefactos, y
re-ejecutó V0 y V1 por su cuenta. Reconstruyó además los md5 de baseline de los
artefactos generados —que no están versionados— compilando una copia del árbol con
el `piece.png` del baseline: los cuatro coincidieron con los declarados en esta
Story, validando la tabla de baseline de forma independiente.

AC1–AC6 verificados con evidencia reproducida. AC7 quedó **implementado pero no
verificado** en el momento de la Review; se cerró después con el acta de V2 (ver
abajo).

### Hallazgos y disposición

| # | Hallazgo | Disposición | Estado |
|---|---|---|---|
| H1 | V2 no ejecutada, sin acta. La clase `render` la exige y un nivel exigido y ausente bloquea el cierre | `decision-needed` → V2 ejecutada y firmada | **resuelto** |
| H2 | Cuatro archivos fuera de la allowlist difieren del baseline (`CLAUDE.md`, `README.md`, `_bmad-output/project-context.md`, `pantalla.txt`), uno en ruta declarada "No modificar" | `decision-needed` → no atribuibles; causa raíz resuelta en `process-1` | **resuelto** |
| H3 | El baseline se declaró sobre un árbol de trabajo ya sucio, lo que vuelve ruidoso por construcción el diff contra baseline. Causa raíz de H2 | `defer` | pendiente de destino durable |
| H4 | Re-ejecutar V0 sobre un árbol ya construido devuelve `No se hace nada para 'all'`: no reproduce nada. La regla de re-ejecución independiente queda vacía para V0 | `defer` | pendiente de destino durable |
| H5 | Estado del loop desactualizado en este archivo (Status y Change Log) | `patch` | **resuelto** |
| H6 | Deuda de validación manual: no aplica, la clase exige V2 | `Rejected by Design` | cerrado |
| H7 | `snes/piece.pic` sin cambios. Confirmado esperado: solo cambió PLTE, datos de píxel byte-idénticos | `Rejected by Design` | cerrado |

### Huecos de verificación declarados por el revisor

- AC7 es perceptual: fuera del alcance de cualquier instrumento programático.
- Los md5 de baseline de artefactos generados se validaron **por reproducción**, no
  por observación directa del estado original.
- No se corrió V1 contra la ROM del baseline; la cadencia de 30 frames se contrastó
  contra la constante del código, no contra una corrida previa registrada.

### Nota de aislamiento

Contexto realmente separado: el revisor no vio esta conversación. Aislamiento
**parcial** respecto a lo que pide el framework: mismo proceso anfitrión y mismo
modelo, el prompt del ejecutor es un canal de encuadre, y la prohibición de escribir
en el árbol fue por instrucción y no por allowlist estructural.

## Acta de V2

**Ejecutada por:** Arturo (orquestador humano). **Fecha:** 2026-07-25.
**Emulador:** Ares. **ROM:** `snes/apotris.sfc`, md5 `25e5dc572d06db5a46f7d40e8e8aa79a`
— la misma que registró V0 en esta Story, y la misma que `rebuild_v0.py` demostró
byte-reproducible desde el contenido de `snes/`.

**Veredicto: PASS.** Las tres condiciones de AC7 se cumplen:

| # | Condición observada | Resultado |
|---|---|---|
| 1 | La pieza activa se dibuja en ámbar, no en cian | **sí** |
| 2 | Sigue cayendo (~1 paso cada medio segundo) | **sí** |
| 3 | Sigue moviéndose con izquierda/derecha del pad | **sí** |

El ámbar se ve cuantizado a 5 bits por canal, como anticipaban las Dev Notes. No
invalida AC1, que se comprueba sobre el PNG fuente.

Con esto **AC7 queda verificado** y H1 cerrado. Los siete AC de la Story están
verificados.

## Cierre de H2

Los cuatro archivos —`CLAUDE.md`, `README.md`, `_bmad-output/project-context.md`,
`pantalla.txt`— **no los tocó esta Story**. Ya diferían del baseline `7c48ac6` antes
de que empezara. La propia Review lo dictaminó así al registrar H3 como causa raíz
de H2.

No se reabre nada: H3 se resolvió en `process-1-baseline-limpio-y-v0-reproducible`,
que construyó el mecanismo de `baseline_dirty` precisamente para este caso, y lo
ejercitó sobre sí mismo declarando estos mismos archivos. Esta Story es anterior al
mecanismo, así que no pudo declararlos — no es un incumplimiento de allowlist, es la
ausencia de una herramienta que ahora existe.

**Disposición: resuelto.** Ninguno de los cuatro entra en el commit de esta Story.

## Change Log

| Fecha | Cambio |
|---|---|
| 2026-07-25 | Story creada (Create Story) |
| 2026-07-25 | Implementación (T1–T4), V0 y V1 en PASS, cadena de assets verificada por md5 |
| 2026-07-25 | Review independiente por subagente: Bloqueado. 7 hallazgos; H5 resuelto (`patch`) |
| 2026-07-25 | V2 ejecutada y firmada por el orquestador humano: PASS. H1 cerrado |
| 2026-07-25 | H2 cerrado: no atribuibles a esta Story; causa raíz H3 resuelta en `process-1`. Story en `done` |
