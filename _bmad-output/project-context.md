---
project_name: 'apotris'
user_name: 'Arturo'
date: '2026-07-10'
sections_completed: ['objetivo', 'stack_tecnico', 'alcance_minimo', 'referencia_codigo_fuente', 'defaults_implementacion', 'anti_patrones']
status: 'complete'
rule_count: 15
optimized_for_llm: true
---

# Project Context for AI Agents

_Reglas críticas para agentes que implementen código de este proyecto: reimplementación nativa de Apotris para SNES._

---

## Objetivo del proyecto

- Meta única: ROM SNES funcional en hardware real. Minimizar esfuerzo humano, complejidad y tokens de agente — no elegancia arquitectónica, no portabilidad.
- Apotris (este repo, GBA/C++/devkitARM) es la base brownfield de referencia. No se busca un port mecánico completo ni preservar su arquitectura GBA/multiplataforma, pero sí se permite reutilizar y adaptar selectivamente código, algoritmos y tablas cuando sea técnicamente viable.

## Stack técnico objetivo

- Lenguaje: C (PVSnesLib) por defecto.
- 65C816 Assembly: solo ante cuello de botella medido (profiling real), nunca preventivo.
- Toolchain/versión exacta de PVSnesLib/devkitSNES: pendiente de fijar al iniciar setup — documentar aquí en cuanto se instale.
- Stack de origen (Apotris): C++ sin RTTI/excepciones, devkitARM, libtonc, libmm/maxmod, ARM7TDMI thumb. **No aplica** al target SNES (SPC700 de audio, otro mapa de memoria/VRAM, sin libtonc/maxmod).

## Alcance mínimo (MVP gameplay)

Implementar únicamente, en este orden: tablero, pieza activa, spawn, movimiento horizontal, gravedad, colisión, lock, siguiente pieza, line clear.

**Fuera de alcance inicial** (no implementar sin pedido explícito): multiplayer por link cable, rumble, Game Boy Player logo, flash saves, audio, modos de juego adicionales (sprint/dig/battle/etc.), replays, skins/personalización.

## Referencia de código fuente (Apotris → mapeo)

Reusar/adaptar selectivamente cuando sea eficiente:
- `include/tetromino.hpp` + `source/tetromino.cpp` — tablas: rotation systems (SRS/NRS/ARS/A-SRS), kicks, gravity, scoring, combo, finesse.
- `include/tetrisEngine.h` + `source/tetrisEngine.cpp` — lógica core: spawn, movimiento, lock, line clear, modos.

GBA-specific, no portar ni usar como referencia de implementación: `rumble.c`, `flashSaves.cpp`, `gbp_logo.cpp`, `LinkConnection.h`/`multiplayer.cpp`, `music.cpp`/`screenAudio.cpp`, `sprites.c` (VRAM GBA).

## Defaults de implementación

- Memoria estática, sin allocación dinámica.
- Lógica determinista por frame (sin dependencia de timing variable).
- Módulos C pequeños, un archivo por responsabilidad clara.
- Preferir reutilización/adaptación selectiva de código/tablas existentes (Apotris) sobre reescritura desde cero, cuando reduzca esfuerzo, complejidad o tokens.

## Anti-patrones para agentes

- No escribir ASM 65C816 sin cuello de botella medido y justificado.
- No portar mecánicamente estructuras C++ (clases, `std::list`, `std::tuple`) de Apotris — no existen en el target C/PVSnesLib. Adaptar la lógica/datos, no la sintaxis.
- No implementar features fuera del alcance mínimo sin pedido explícito.
- No generar documentación extensa, análisis exhaustivos ni sub-agentes para tareas de este proyecto salvo que se pida.

---

## Usage Guidelines

**Para agentes IA:**

- Leer este archivo antes de implementar cualquier código de juego.
- Seguir todas las reglas tal como están documentadas.
- Ante duda, preferir la opción más restrictiva (menor alcance, menor complejidad).

**Para humanos:**

- Mantener este archivo compacto, enfocado en necesidades de agentes.
- Actualizar cuando cambie el stack técnico o el alcance.
- Eliminar reglas que se vuelvan obvias con el tiempo.

Last Updated: 2026-07-10

## Token Budget

Para este proyecto:

- Preferir workflows pequeños e incrementales.
- Evitar subagentes salvo que el usuario los solicite.
- Evitar investigaciones web cuando el problema pueda resolverse inspeccionando el repositorio o el entorno local.
- Cada Story debe poder implementarse en una sola sesión.
- Antes de usar workflows de revisión o investigación costosos, proponer una alternativa de bajo consumo de tokens.

## Validation

Cada Story debe terminar en este orden:

1. make
2. Ejecutar en ares
3. Esperar validación manual del desarrollador
4. Solo entonces considerar la Story terminada

Nunca asumir que una Story está completa únicamente porque compila.

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

Nunca intentar encontrar la causa raíz completa en una sola respuesta.

El objetivo es reducir el espacio de búsqueda, no resolver todo el problema.

Después de cada hipótesis confirmada o descartada, detenerse y esperar la siguiente instrucción del desarrollador.

## Investigaciones externas

Una investigación mediante `/gds-investigate` resuelve la pregunta planteada.

Una vez obtenida una respuesta suficiente, no volver a investigar el mismo tema desde otro ángulo salvo que exista evidencia nueva que contradiga la conclusión.

Cuando una Story modifica únicamente assets gráficos:

La validación debe confirmar que:

- el asset generado cambió;
- el objeto que lo incorpora se regeneró;
- la ROM cambió realmente.

No asumir que `make` recompiló los recursos solo porque terminó sin errores.