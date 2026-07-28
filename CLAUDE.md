# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

**SNES Technical Demo** — a native Super Nintendo (65816) technical demonstration written from scratch in C on PVSnesLib. The running program is a block-stacking playfield; the engineering substance is the architecture, the hardware constraints, and the validation process.

Game logic is informed by [Apotris](https://akouzoukos.itch.io/apotris) (a guideline-compliant block-stacking game written for GBA in C++/devkitARM/libtonc, GPLv3). This is **not** a mechanical port — algorithms and data tables are selectively adapted where technically viable, and each adapting function cites the original in a source comment. Do not present the project under the Apotris name; treat it as historical origin only.

**Current state:** the minimum declared scope — board, active piece, spawn, horizontal movement, gravity, collision, lock, next piece, line clear — is implemented and building. `snes/` holds nine C modules plus assets; `make` produces a 256 KB ROM. The toolchain (PVSnesLib + wla-dx + gfx4snes) is installed and in daily use. The last work unit is awaiting V2 sign-off. There is still **no unit test suite, no linter and no CI** — verification is V0/V1/V2 as described under "Validation".

Build artefacts are still named `apotris.sfc` / `apotris.sym` (`ROMNAME` in `snes/Makefile`). Historical; renaming is a build change, not a documentation change.

All planning docs are written in Spanish; expect Spanish in `_bmad-output/*.md` and `docs/*.md`. Source code, code comments and `README.md` are in English.

## Repository layout

- `snes/` — target SNES project root (C, PVSnesLib). All game code goes here.
  - `snes/source/` — `board.c`, `piece.c`, `piece_data.c`, `queue.c`, `input.c`, `render.c`, `main.c`, `test_status.c`, `test_runner.c` and their headers. Ownership rules in `game-architecture.md`: `render.c` is the only module touching VRAM/OAM, `input.c` the only one touching pad symbols, `board.c` the sole writer of the board array, and `main.c` holds no game rule.
  - `snes/*.png`, `Makefile`, `hdr.asm`, `data.asm` — source art and build inputs. Generated artefacts (`.sfc`, `.sym`, `.obj`, `.pic`, `.pal`, `.map`, `.inc`, `*_data.as`, `linkfile`) are gitignored and never edited by hand.
- `tools/` — `harness/` (V1 orchestrator, Python), `lua/` (probe running inside BizHawk), `loop/` (`story_baseline.py` scope gate, `rebuild_v0.py` reproducible build). The BizHawk install under `tools/BizHawk-*/` is gitignored (243 MB).
- `docs/` — Native Loop framework, its game-dev adaptation layer, and the engineering case study.
- `reference/apotris/` — full original GBA source (C++/devkitARM/libtonc), kept locally as a reference only. **Gitignored, not tracked** — do not assume it's present in a fresh clone. Read it for logic to adapt, never build/run it as part of this project, and never copy its C++ constructs verbatim.
- `_bmad-output/` — planning artifacts produced via BMAD workflows; the source of truth for rules, architecture, and scope (see "Required Reading" below).
- `_bmad/` — BMAD tool config, not project source.
- `BOOTSTRAP.md` — single entry point for verification commands, path conventions and current process state.

## Required Reading

Before implementing any Story, read these in order — they are the source of truth, not this file:

1. `_bmad-output/project-context.md` — binding rules/anti-patterns for this project.
2. `_bmad-output/game-architecture.md` — target C module design, ownership, frame-loop order, VRAM strategy.
3. `_bmad-output/planning-artifacts/epics.md` — full FR/NFR list and epic/story breakdown with acceptance criteria.
4. The corresponding Story file in `_bmad-output/implementation-artifacts/` — the detailed, ready-for-dev spec for the current Story.

Do not modify `project-context.md` or any other BMAD artifact — those are maintained through BMAD workflows, not directly.

## Development Philosophy

- Implement only the current Story. Do not get ahead of future Stories.
- Prefer the smallest implementation that satisfies the Story's acceptance criteria.
- Avoid speculative abstractions.
- Before writing a new algorithm, check `reference/apotris/` for an equivalent implementation first.
- Prefer adapting a proven algorithm over reinventing one, when it reduces effort and preserves the desired behavior.
- Never do a mechanical C++ → C translation — adapt the logic/data, not the syntax (classes/`std::list`/`std::tuple` have no equivalent in C).

## Git workflow

- Claude never executes Git commands that modify repository history **on its own initiative**.
- Default: never run `git add`, `git commit`, `git push`, `git pull --rebase`, `git merge`, `git rebase`, `git tag`, or create/delete branches. Suggest the command instead.
- **Exception — explicit, per-instance authorization.** Arturo may authorize a specific Git operation in the current message. That authorization covers only the operation named, only once; it does not carry over to the next turn or to a different operation.
- `git push`, `git tag` and history rewrites (`rebase`, `reset --hard`, `commit --amend` on pushed commits) stay off-limits regardless: those reach outside the local repository or destroy work.
- Never finish a workflow by committing or pushing automatically. A commit happens because it was asked for in that message, never as a workflow's closing step.
- When authorized to commit, stage the **attributable set** of the Story, not `git add -A`. Pre-existing dirt declared in `baseline_dirty` belongs to someone else's pending work and must stay uncommitted.

## Token Budget

Para este proyecto:

- Preferir workflows pequeños e incrementales.
- Evitar subagentes salvo que el usuario los solicite.
- Evitar investigaciones web cuando el problema pueda resolverse inspeccionando el repositorio o el entorno local.
- Cada Story debe poder implementarse en una sola sesión.
- Antes de usar workflows de revisión o investigación costosos, proponer una alternativa de bajo consumo de tokens.

## Validation

El nivel mínimo de validación de cada Story lo determina su **clase**, declarada durante `/gds-create-story`. La política completa y la tabla de clasificación viven en `docs/BMAD_GAMEDEV_NATIVE_LOOP.md` — esa es la fuente de verdad.

Los tres niveles:

- **V0** — `make` en `snes/`.
- **V1** — `python3 tools/harness/harness.py`. Harness automático; no requiere intervención humana.
- **V2** — validación manual en el emulador por el desarrollador.

Reglas operativas:

1. Toda Story cuyo diff pueda alterar la ROM exige **V0 + V1** como mínimo.
2. **V2 ya no es obligatoria en cada Story.** Solo la exigen las clases `render`, `audio`, `ux-hud`, `rendimiento`, `integracion`, `gate-epic` y `gate-rc`.
3. Las Stories de lógica interna — clases `logica-interna`, `algoritmos`, `ia`, `contratos`, `herramientas` — se cierran con **V0 + V1**, sin abrir el emulador.
4. Una Story de clase `documentacion` se cierra con **V0**: no puede alterar la ROM, así que V1 no aportaría información.
5. El ejecutor no puede cambiar la clase declarada. Si cree que está mal, proponer `/gds-correct-course`.
6. Toda Story cerrada sin V2 anota su deuda de validación manual, que el cierre de Epic consume. **El destino durable de ese registro está pendiente de definir** (ver "Requisitos para adopción" en `docs/BMAD_GAMEDEV_NATIVE_LOOP.md`); hasta que exista, anotarla en la propia Story.

Nunca asumir que una Story está completa únicamente porque compila. Tampoco porque el harness dio PASS: la Review independiente re-ejecuta V0 y V1 por su cuenta, y es quien cierra la Story.

Durante gameplay, la prioridad de validación es **V0 → V1 → V2**. La inspección estática complementa; no sustituye. Un hallazgo que solo sale de leer código, sin ejecución que lo respalde, se marca **no reproducido** y baja de severidad.

## Nivel de Review

Se deriva de la clase, igual que el nivel de validación. **No lo elige el revisor.** Tabla completa en `docs/BMAD_GAMEDEV_NATIVE_LOOP.md`.

| Clase | Nivel |
|---|:---:|
| `herramientas`, `contratos`, `rendimiento`, `gate-epic`, `gate-rc` | **A** — profunda |
| `integracion`, `algoritmos`, `logica-interna`, `ia`, `render` | **B** — media |
| `ux-hud`, `audio`, `documentacion` | **C** — ligera |

**Escalado automático a A**, con independencia de la clase, si el conjunto atribuible: cambia el layout de WRAM (variable global o `static` nueva, quitada o redimensionada), toca DMA/VRAM/OAM o el mapa de memoria, toca `tools/`, o toca el `Makefile`.

Los tres niveles son **independientes del ejecutor** y los tres **re-ejecutan V0, V1 y la contención de alcance**. Lo que varía es la profundidad de la investigación:

- **A** — investigación profunda. Audita cada AC contra el código, busca regresiones en sistemas colindantes, intenta romper deliberadamente, analiza arquitectura, y puede crear instrumentación temporal. Invierte el tiempo necesario.
- **B — acotada.** Verifica cuatro cosas: AC cumplidos, no rompe lo existente, alcance respetado, sin regresiones evidentes. Se apoya en **V0 → V1 → diff → código modificado**, en ese orden. **No** lee módulos no relacionados, **no** revisa arquitectura completa, **no** propone rediseños, **no** escribe herramientas ni scripts ni sondas, **no** hace investigación exploratoria. Una mejora fuera de alcance se registra como `defer` y **no se investiga**.
- **C — extremadamente ligera.** Solo diff, AC y resultado de V0/V1. Cero investigación adicional.

La Review responde **una sola pregunta: ¿la Story cumple el objetivo definido por sus AC?** **No es una auditoría del proyecto.** No rediseña la arquitectura, no amplía el alcance, y no crea herramientas de depuración fuera del nivel A.

**Principio de coste:** la profundidad es proporcional al riesgo, y el esfuerzo de Review no debe superar claramente el de implementación. Implementación de minutos → Review de minutos.

**Exento del principio de coste:** V0, V1 y la contención de alcance se corren **siempre**, en los tres niveles. Su coste es tiempo de máquina, no atención. El principio gobierna la investigación, no la ejecución.

**Escalar una Review lo decide el Loop, no el revisor.** Si un revisor de nivel B o C cree que hace falta investigación profunda, **se detiene de inmediato y explica por qué** — qué encontró, por qué no puede concluir, y qué haría si se le autoriza. No la empieza por su cuenta.

## Sprint Mode

Ejecución continua. Al cerrar una Story correctamente, y **sin pedir autorización**: commit acotado al conjunto atribuible → actualizar la Story → actualizar el roadmap si aplica → `story_baseline.py snapshot` → siguiente Story.

Se detiene **solo** por: V2 requerida, decisión arquitectónica, blocker real, o presupuesto agotado. Los hallazgos `patch` se aplican y se sigue; los `defer` se registran y se sigue.

El presupuesto se fija al abrir el Sprint —N Stories, hasta la primera V2, o tokens—. Sin presupuesto declarado, por defecto es **hasta la primera V2**. Al agotarse, entregar el resumen de Sprint completo.

## Investigación externa (PVSnesLib y toolchains)

Por defecto:

- NO usar Internet.
- NO investigar documentación externa.
- Resolver usando:
  - project-context.md
  - game-architecture.md
  - epics.md
  - reference/apotris
  - ejemplos oficiales instalados localmente.

EXCEPCIÓN:

Si después de una investigación local:

- se descartaron al menos 2 hipótesis razonables;
- el problema pertenece a una librería/framework/toolchain externo
  (PVSnesLib, devkitSNES, gfx4snes, etc.);
- y no existe evidencia concluyente en el código local;

DETENER la implementación.

No seguir proponiendo hipótesis.

En su lugar, sugerir explícitamente ejecutar:

/gds-investigate

con una búsqueda externa limitada al problema técnico específico.

La investigación debe:

- responder una única pregunta;
- usar preferentemente documentación oficial;
- entregar la referencia utilizada;
- proponer el cambio mínimo.

No volver a investigar lo ya descartado.

## Creación de Stories

Al ejecutar `/gds-create-story`:

- El objetivo es producir la Story, no investigar la implementación.
- No realizar inventarios de archivos, funciones o llamadas salvo que sean imprescindibles para definir el alcance.
- No volver a confirmar información ya establecida en Stories, arquitectura o decisiones previas.
- Mantener el consumo de tokens al mínimo.
- Dejar el análisis detallado del código para `/gds-dev-story` o `/gds-code-review`.

## Límite de investigación

Las investigaciones deben ser de alcance mínimo.

Reglas:

- Formular una única hipótesis por iteración.
- Intentar refutar o confirmar únicamente esa hipótesis.
- No abrir nuevas líneas de investigación durante la misma respuesta.
- Si la hipótesis no puede confirmarse con evidencia directa del código inspeccionado, detenerse.

No continuar ampliando el análisis automáticamente.

Esperar instrucciones del desarrollador para la siguiente hipótesis.

No seguir la cadena de dependencias.

Ejemplo:

main.c
→ render.c
→ PVSnesLib
→ código fuente de PVSnesLib

La investigación debe detenerse en el primer límite del proyecto.

Si para responder es necesario inspeccionar una dependencia externa o una librería, detenerse y sugerir `/gds-investigate`.

Antes de ampliar una investigación, responder:

- ¿Qué evidencia nueva espero obtener?
- ¿Cambiará una decisión de implementación?

Si ambas respuestas no son "sí", detener la investigación.

## Investigaciones externas

Una investigación mediante `/gds-investigate` resuelve la pregunta planteada.

Una vez obtenida una respuesta suficiente, no volver a investigar el mismo tema desde otro ángulo salvo que exista evidencia nueva que contradiga la conclusión.

Cuando una Story modifica únicamente assets gráficos:

La validación debe confirmar que:

- el asset generado cambió;
- el objeto que lo incorpora se regeneró;
- la ROM cambió realmente.

No asumir que `make` recompiló los recursos solo porque terminó sin errores.