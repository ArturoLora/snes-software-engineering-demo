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
