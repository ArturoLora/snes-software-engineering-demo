---
baseline_commit: e4e50a797c7646da66e0852e344d448ae2d606be
---

# Story 1.1: Boot de ROM e inicialización mínima de PVSnesLib

Status: in-progress (implementación + build verificados; validación manual en emulador pendiente — ver Completion Notes)

<!-- Note: Validation is optional. Run validate-create-story for quality check before dev-story. -->

## Story

Como desarrollador,
quiero que la ROM arranque, inicialice PVSnesLib, entre al game loop determinista, y verifique consola de texto + lectura de pad,
para tener una base ejecutable y verificable en emulador/hardware SNES antes de escribir cualquier lógica de juego (board/pieza/render).

## Alcance (fijado explícitamente por el usuario — reemplaza/amplía el AC original de epics.md §Story 1.1)

**Sí implementar:**
- Toolchain PVSnesLib funcional (ROM compila).
- Init de PVSnesLib (modo de video, pantalla activa).
- Entrar al game loop (`while(1)`).
- `WaitForVBlank()` cada frame.
- Consola de texto inicializada (`consoleInitText`).
- Mensaje tipo "Hello" visible en pantalla.
- Lectura de pad y despliegue de info básica en pantalla (verificación de input).

**NO implementar en esta story** (explícitamente fuera de alcance, aunque aparezcan en `game-architecture.md`):
board, pieza activa, spawn, render gráfico real, BG del playfield, sprites OAM del juego, DMA, colisiones, gravedad, 7-bag, rotación, cualquier lógica de juego. El BG que usa `consoleInitText` es solo para texto de depuración — **no** es el playfield BG (eso es Story 1.2).

## Acceptance Criteria

1. **Given** el toolchain PVSnesLib instalado y `PVSNESLIB_HOME` configurado, **when** se ejecuta `make` en el proyecto SNES, **then** se genera un archivo `.sfc` sin errores de compilación/link.
2. **Given** la ROM compilada, **when** se carga en un emulador SNES (bsnes/higan/Mesen-S/Snes9x), **then** no crashea ni cuelga y llega a un estado visual definido (`setScreenOn()`).
3. **Given** la ROM corriendo, **when** arranca, **then** se ve un mensaje de texto (ej. "Hello Apotris SNES") vía `consoleInitText`/`consoleDrawText`.
4. **Given** la ROM corriendo, **when** el jugador presiona un botón del pad, **then** la pantalla de texto refleja el estado del input leído ese frame (confirma que la lectura de pad funciona).
5. **Given** el loop principal, **when** transcurre cada iteración, **then** se llama `WaitForVBlank()` exactamente una vez por vuelta (frame loop determinista, sin busy-wait fuera del VBlank).

## Tasks / Subtasks

- [x] **Task 1: Instalar/configurar toolchain PVSnesLib (prerequisito de entorno, no código)** (AC: #1)
  - [x] Descargado release 4.5.0 (más reciente que la 4.3.0 asumida en la story) desde `https://github.com/alekmaul/pvsneslib/releases/latest`.
  - [x] `PVSNESLIB_HOME` exportado apuntando al directorio extraído.
  - [x] **Verificado:** `816-tcc`/`wla-65816`/`wlalink`/`gfx4snes` (GFXCONV) accesibles bajo `devkitsnes/bin` y `devkitsnes/tools`; smoke-test compilando el ejemplo oficial `hello_world` confirmó el toolchain funcional antes de escribir código propio. **Instalado solo en este entorno de sesión (`~/pvsneslib_install`), no en el repo** — ver Completion Notes.

- [x] **Task 2: Crear árbol de proyecto SNES nuevo, aislado del código GBA existente** (AC: #1)
  - [x] Creado `snes/` en la raíz del repo.
  - [x] Dentro de `snes/`: `source/main.c`, `hdr.asm`, `data.asm`, `pvsneslibfont.png`, `Makefile`.
  - [x] `hdr.asm`/`data.asm`/`Makefile`/`pvsneslibfont.png` copiados tal cual del ejemplo oficial `hello_world` (release 4.5.0 instalada). Cambios: `NAME` en `hdr.asm` → `"APOTRIS SNES         "` (21 bytes exactos), `ROMNAME` en `Makefile` → `apotris`, y `SRC := source` agregado antes del `include snes_rules` (el default de `snes_rules` es `src`, la story pide `source/`).

- [x] **Task 3: `snes/source/main.c` — init + mensaje + lectura de pad** (AC: #2, #3, #4, #5)
  - [x] Secuencia de init implementada en el orden especificado (idéntica a `hello_world.c`).
  - [x] Loop principal: `while(1) { padsCurrent(0); consoleDrawText(...); WaitForVBlank(); }` — una sola llamada a `WaitForVBlank()` por vuelta.
  - [x] Lectura de pad con `padsCurrent(0)` + `switch` sobre `KEY_A/B/SELECT/START/RIGHT/LEFT/DOWN/UP/R/L/X/Y` (patrón calcado de `snes-examples/input/controller/controller.c`), dibujado en fila fija (6,18) sobrescrita cada frame.
  - [x] Sin `input.c`: la lectura de pad es inline en `main.c`, tal como especifica la story.

- [x] **Task 4: Compilar y verificar en emulador** (AC: #1, #2, #3, #4, #5)
  - [x] `make` en `snes/` → produce `snes/apotris.sfc` sin errores (AC #1 verificado automáticamente).
  - [ ] Cargar en un emulador SNES y confirmar AC #2, #3, #4, #5 — **pendiente de validación manual del usuario** (sin emulador disponible en este entorno; ver Completion Notes para instrucciones).

## Dev Notes

### Decisiones de alcance y arquitectura

- Esta story **amplía** el AC original de `epics.md` Story 1.1 (que solo pedía `setScreenOn()` + color sólido) por pedido explícito del usuario: se agrega consola de texto + mensaje + verificación de pad, todo dentro del mismo boot mínimo. El BG del playfield real (`bgInitTileSet`/`bgInitMapSet` del tablero 10×20) sigue siendo Story 1.2, sin tocar en esta story.
- Frame loop determinista por `game-architecture.md` §5/§9: la única regla aplicable a esta story es "loop + `WaitForVBlank()` al final"; el resto de la sección 5 (spawn, gravedad, lock, line-clear) no aplica todavía.

### Project Structure Notes

- **Conflicto detectado:** `game-architecture.md` §1 describe el árbol como `source/main.c`, `source/game_state.h`, etc., pero la raíz del repo **ya tiene** `source/` y `Makefile` ocupados por el port GBA/C++ existente (`source/main.cpp`, `tetrisEngine.cpp`, etc.), que usa un toolchain distinto (devkitARM/libtonc) e incompatible con 816-tcc.
- **Resolución (esta story):** el árbol de `game-architecture.md` §1 vive bajo un nuevo directorio raíz `snes/` (`snes/source/main.c`, y en stories futuras `snes/source/board.c`, etc.), no en la raíz del repo. Esto evita pisar el código GBA existente y mantiene ambos toolchains separados. Las próximas stories deben seguir usando `snes/` como raíz del proyecto SNES.

### Library/Framework Requirements

- PVSnesLib (`alekmaul/pvsneslib`, rama `master`, release estable más reciente conocida: 4.3.0). Instalación no verificada localmente en este repo — Task 1 lo cubre como prerequisito de entorno.
- Toolchain: `816-tcc` (C puro, sin C++/STL — restricción dura del compilador, no solo preferencia).
- API confirmada por código fuente real de ejemplos oficiales (no inventada):
  - `snes-examples/hello_world/src/hello_world.c` → secuencia de init + `consoleDrawText` + loop.
  - `snes-examples/input/controller/controller.c` → `padsCurrent(0)`, constantes `KEY_*`, patrón de `consoleDrawText` refrescado en loop.
  - `snes-examples/hello_world/{hdr.asm,data.asm,Makefile}` → boilerplate de cabecera SNES y build.
- **[VERIFICAR EN IMPLEMENTACIÓN]**: `game-architecture.md` §6 documenta `pad_keys[5]`/`pad_keysdown[5]` (poblados por VBlank-ISR) como la API que usará el futuro `input.c`; esta story usa `padsCurrent(0)` (API distinta, más simple, confirmada en el ejemplo oficial de input) solo para la verificación de boot. Si al implementar `padsCurrent()` no existe o difiere, usar `pad_keys[0]` directo como fallback — cualquiera de las dos sirve para el único propósito de esta story (probar que el input llega).

### Project Context Rules (de `project-context.md`)

- Sin asignación dinámica de memoria; estructuras estáticas únicamente (no aplica aún — esta story no define structs de juego).
- C puro, sin C++/STL (toolchain 816-tcc no los soporta).
- No escribir ASM 65C816 salvo cuello de botella medido — no aplica en esta story (solo el `hdr.asm`/`data.asm` boilerplate estándar de todo proyecto PVSnesLib, no ASM de lógica).
- No implementar features fuera del alcance mínimo — respetar estrictamente la lista de "NO implementar" arriba, aunque `game-architecture.md` las describa para stories futuras.
- Módulos C pequeños, un archivo por responsabilidad — en esta story solo existe `main.c` (sin módulos aún, es boot puro).

### References

- [Source: _bmad-output/game-architecture.md#9 Inicialización mínima de ROM/PVSnesLib]
- [Source: _bmad-output/game-architecture.md#6 Input PVSnesLib]
- [Source: _bmad-output/game-architecture.md#1 Estructura mínima de archivos]
- [Source: _bmad-output/planning-artifacts/epics.md#Story 1.1: Boot de ROM e inicialización mínima de PVSnesLib]
- [Source: _bmad-output/project-context.md#Stack técnico objetivo, Anti-patrones para agentes]
- [Source: _bmad-output/implementation-artifacts/investigations/pvsneslib-mvp-cross-deps-investigation.md — toolchain 816-tcc confirmado, sin instalación local]
- [Source (código oficial, verificado por fetch directo en esta sesión): github.com/alekmaul/pvsneslib `snes-examples/hello_world/src/hello_world.c`, `snes-examples/input/controller/controller.c`, `snes-examples/hello_world/{hdr.asm,data.asm,Makefile}`]

## Dev Agent Record

### Agent Model Used

claude-sonnet-5

### Debug Log References

- Build log (release 4.5.0, `make` en `snes/`): compiló y linkeó sin errores tras corregir la longitud del campo `NAME` en `hdr.asm` (WLA-DX exige exactamente 21 bytes; el primer intento con padding incorrecto falló con `DIRECTIVE_ERROR: NAME requires a string of 1 to 21 letters`).
- Smoke-test previo: se compiló el ejemplo oficial `snes-examples/hello_world` sin modificar, para confirmar que el toolchain (816-tcc/wla-65816/wlalink/gfx4snes) funciona en este SO antes de escribir código propio.

### Completion Notes List

- **Toolchain no queda instalado en el repo ni en el entorno del usuario.** Se descargó y extrajo PVSnesLib 4.5.0 (release oficial más reciente; la story asumía 4.3.0, no existe conflicto de API relevante para esta story) en `~/pvsneslib_install` **de esta sesión/sandbox únicamente**. Antes de poder correr `make` en `snes/` en otra máquina (la del usuario, CI, etc.) hace falta:
  1. Descargar `pvsneslib_450_64b_linux.zip` (o el binario del SO correspondiente) desde `https://github.com/alekmaul/pvsneslib/releases/latest` y extraerlo.
  2. `export PVSNESLIB_HOME=<ruta al directorio extraído>/pvsneslib` (la ruta termina en un directorio `pvsneslib` anidado dentro del zip).
  3. Opcional: agregar `<PVSNESLIB_HOME>/devkitsnes/bin` al `PATH`.
- **Build verificado (AC #1):** `make` en `snes/` genera `snes/apotris.sfc` (262144 bytes) sin errores ni warnings de link. Se corrió `make clean` al final, por lo que el árbol `snes/` quedó solo con los fuentes (`hdr.asm`, `data.asm`, `Makefile`, `pvsneslibfont.png`, `source/main.c`) — sin `.sfc`/`.sym`/`.obj`/artefactos de `gfx4snes`, todos regenerables con `make`.
- **AC #2–#5 no verificados por el agente** — no hay emulador SNES ni entorno gráfico en este sandbox, y el usuario pidió explícitamente no instalar ninguno ni modificar el sistema. Validación manual pendiente. Pasos para el usuario:
  1. Instalar el toolchain (ver arriba) y correr `make` dentro de `snes/` → debe producir `snes/apotris.sfc`.
  2. Abrir `snes/apotris.sfc` en bsnes, Mesen-S o Snes9x.
  3. **AC #2** (no crashea/cuelga): la pantalla debe llegar a un estado visual estable (fondo con texto), no quedar en negro ni congelada.
  4. **AC #3** (mensaje visible): debe verse el texto `"Hello Apotris SNES"` y `"PRESS A PAD BUTTON"` en pantalla.
  5. **AC #4** (lectura de pad): al presionar cualquier botón del pad (A/B/X/Y/L/R/Start/Select/D-Pad), debe aparecer una línea tipo `"<BOTÓN> PRESSED"` en la fila inferior, y desaparecer (línea en blanco) al soltar todos los botones.
  6. **AC #5** (frame loop determinista): confirmado por inspección de código — `main.c` tiene una única llamada a `WaitForVBlank()` al final del `while(1)`, sin otra espera/busy-wait en el loop.
- **Sin desviaciones de alcance:** no se tocó nada fuera de lo permitido por la story (no hay `board`/`piece`/render real/BG del playfield/DMA). No se creó `input.c` (lectura de pad queda inline en `main.c`, como pide la story).
- **Una desviación menor:** la story sugiere PVSnesLib 4.3.0; se usó 4.5.0 (release actual al momento de esta implementación) porque es la que ofrece GitHub Releases hoy. La API usada (`consoleInitText`, `consoleDrawText`, `padsCurrent`, `KEY_*`, `bgSetGfxPtr`, `setMode`, `WaitForVBlank`) es la misma verificada en los ejemplos oficiales incluidos en esa release — sin cambios de firma detectados.

### File List

- `snes/hdr.asm` (nuevo) — cabecera SNES/WLA-DX, adaptada del ejemplo oficial `hello_world` (solo `NAME` cambiado).
- `snes/data.asm` (nuevo) — sección rodata con el tileset/paleta de la fuente de consola, sin cambios respecto al ejemplo oficial.
- `snes/Makefile` (nuevo) — adaptado del ejemplo oficial `hello_world`: `ROMNAME := apotris` y `SRC := source` (en vez del `src` por defecto de `snes_rules`).
- `snes/pvsneslibfont.png` (nuevo) — asset de fuente de consola, copiado tal cual del ejemplo oficial `hello_world`.
- `snes/source/main.c` (nuevo) — boot, init de PVSnesLib, consola de texto, mensaje y lectura de pad inline.

### Change Log

| Fecha | Cambio |
| --- | --- |
| 2026-07-11 | Implementación inicial de Story 1.1: árbol `snes/` creado, toolchain PVSnesLib 4.5.0 instalado y verificado en el entorno de sesión, `make` produce `snes/apotris.sfc` sin errores (AC #1). AC #2–#5 quedan pendientes de validación manual del usuario en emulador. Status → in-progress. |
