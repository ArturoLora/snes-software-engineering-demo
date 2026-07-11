# Story 1.1: Boot de ROM e inicialización mínima de PVSnesLib

Status: ready-for-dev

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

- [ ] **Task 1: Instalar/configurar toolchain PVSnesLib (prerequisito de entorno, no código)** (AC: #1)
  - [ ] Descargar release 4.3.0 (o la última estable) desde `https://github.com/alekmaul/pvsneslib/releases/latest`, o clonar+compilar desde `alekmaul/pvsneslib` (rama `master`) siguiendo `https://github.com/alekmaul/pvsneslib/wiki/Installation`.
  - [ ] Exportar `PVSNESLIB_HOME` apuntando al directorio instalado/extraído.
  - [ ] **[VERIFICAR EN IMPLEMENTACIÓN]** confirmar que `816-tcc`/`wla-dx`/`GFXCONV` quedan accesibles y que un `make` trivial corre en este SO (Linux) antes de escribir código de juego — no hay instalación local previa verificada en este repo.

- [ ] **Task 2: Crear árbol de proyecto SNES nuevo, aislado del código GBA existente** (AC: #1)
  - [ ] Crear directorio nuevo `snes/` en la raíz del repo — **no reusar** `source/`/`Makefile` de la raíz (son C++/devkitARM/libtonc del port GBA, incompatibles con el toolchain 816-tcc de PVSnesLib). Ver "Project Structure Notes" abajo.
  - [ ] Dentro de `snes/`: `source/main.c`, `hdr.asm`, `data.asm`, asset de fuente (`pvsneslibfont.png` o `.bmp`), `Makefile`.
  - [ ] `hdr.asm`/`data.asm`/`Makefile`/asset de fuente: copiar tal cual el patrón del ejemplo oficial `pvsneslib/snes-examples/hello_world/` (boilerplate de cabecera SNES/WLA-DX, secciones de rodata con `.incbin` del tileset+paleta de fuente, reglas `GFXCONV`) — **no reinventar** estos archivos, son plantilla estándar de todo proyecto PVSnesLib. Cambiar solo `NAME` en `hdr.asm` y `ROMNAME` en `Makefile` (ej. `apotris`).

- [ ] **Task 3: `snes/source/main.c` — init + mensaje + lectura de pad** (AC: #2, #3, #4, #5)
  - [ ] Secuencia de init (orden confirmado, `game-architecture.md` §9 + patrón oficial de `hello_world.c`/`controller.c`):
    1. `consoleSetTextMapPtr(0x6800); consoleSetTextGfxPtr(0x3000); consoleSetTextOffset(0x0100);`
    2. `consoleInitText(0, 16*2, &<fontTiles>, &<fontPal>);`
    3. `bgSetGfxPtr(0, 0x2000); bgSetMapPtr(0, 0x6800, SC_32x32);`
    4. `setMode(BG_MODE1, 0);`
    5. `bgSetDisable(1); bgSetDisable(2);` (BGs no usados)
    6. `consoleDrawText(x, y, "Hello Apotris SNES");` (o texto similar)
    7. `setScreenOn();`
  - [ ] Loop principal: `while(1) { <leer pad y refrescar texto>; WaitForVBlank(); }`.
  - [ ] Lectura de pad: usar `padsCurrent(0)` (confirmado en `snes-examples/input/controller/controller.c` — retorna máscara de botones, comparar contra `KEY_A/KEY_B/KEY_START/KEY_LEFT/...`) y volcar el resultado con `consoleDrawText` en una posición fija (sobrescribiendo el texto anterior cada frame).
  - [ ] **Importante:** esto es una lectura de pad *inline* en `main.c` solo para verificar el toolchain — **no** es el módulo `input.c` de la arquitectura (ese llega en Epic 4/Story 4.2 y usará `pad_keys`/`pad_keysdown` con la capa `InputIntent`). No crear `input.c` en esta story.

- [ ] **Task 4: Compilar y verificar en emulador** (AC: #1, #2, #3, #4, #5)
  - [ ] `make` en `snes/` → produce `snes/apotris.sfc` (o el nombre elegido) sin errores.
  - [ ] Cargar en un emulador SNES y confirmar los 5 AC de arriba.

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

### Debug Log References

### Completion Notes List

### File List
