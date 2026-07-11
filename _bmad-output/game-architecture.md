---
title: 'Game Architecture (MVP núcleo gameplay)'
project: 'apotris-snes'
date: '2026-07-10'
author: 'Arturo'
version: '1.0'
status: 'complete'
scope: 'tablero, pieza activa, spawn, movimiento horizontal, gravedad, colisión, lock, siguiente pieza, line clear — sin rotación (fuera de este MVP)'
source_documents:
  - '_bmad-output/project-context.md'
  - '_bmad-output/deep-dive-tetris-core.md'
  - '_bmad-output/implementation-artifacts/investigations/pvsneslib-mvp-cross-deps-investigation.md'
---

# Arquitectura técnica mínima — MVP SNES (Apotris → C/PVSnesLib)

Nota de alcance: la lista de sistemas del MVP (tablero, pieza activa, spawn, movimiento horizontal, gravedad, colisión, lock, siguiente pieza, line clear) **no incluye rotación**. No se diseña input ni tablas de rotación/kicks aquí. De `GameInfo::tetraminos[7][4][4][4]` (Apotris) solo se usa el índice de rotación 0 (forma de spawn).

## 1. Estructura mínima de archivos

```
source/
  main.c          -- init PVSnesLib/ROM, bucle principal, orquesta el orden de la sección 5
  game_state.h    -- struct raíz GameState (sin lógica), contrato entre módulos
  board.c/.h      -- tablero: colisión, escritura de celdas, detección/colapso de líneas
  piece.c/.h      -- pieza activa: spawn, movimiento horizontal, gravedad, lock
  piece_data.c/.h -- tablas estáticas: forma rotación-0 por pieza, tabla de gravedad Q8.8 (adaptadas de Apotris)
  queue.c/.h      -- bag[7] + cola de siguiente pieza (usa rand())
  input.c/.h      -- traduce pads PVSnesLib a un InputIntent neutral
  render.c/.h     -- único módulo con VRAM/OAM: tilemap del playfield + sprites de la pieza activa
```

## 2. Módulos C y responsabilidad

| Módulo | Responsabilidad | No hace |
| --- | --- | --- |
| `main.c` | Init hardware/PVSnesLib, bucle `while(1)` con el orden de la sección 5, `WaitForVBlank()` | Ninguna regla de juego |
| `board.c` | Dueño de `board[][]`; colisión (`board_is_cell_occupied`), escritura de celda, detección de filas completas, colapso | No conoce pads ni VRAM |
| `piece_data.c` | Tablas estáticas reutilizadas/adaptadas de Apotris (forma por pieza, gravedad Q8.8) | Sin estado mutable |
| `piece.c` | Dueño de la pieza activa: spawn, movimiento horizontal (DAS/ARR), gravedad, lock-delay, invoca `board_*` para colisión/escritura | No decide colores ni dibuja nada |
| `queue.c` | Dueño de `bag`/`queue`; único módulo que llama `rand()` | No conoce reglas de colisión |
| `input.c` | Única frontera con `pad_keys`/`pad_keysdown` de PVSnesLib | No conoce `GameState` |
| `render.c` | Único módulo que incluye headers de video de PVSnesLib (`background.h`, `oam.h`); lee `GameState` de solo lectura | Nunca escribe `GameState` |

## 3. Ownership del estado

Un único struct raíz `GameState` (en `game_state.h`) agrupa `BoardState`, `ActivePiece`, `PieceQueue`, `GravityState`, `LinesToClear`. Se pasa por puntero a los módulos que lo necesitan. Regla de propiedad: cada módulo `*.c` es el **único** que escribe los campos de su sub-struct correspondiente (`board.c` escribe `BoardState`, `piece.c` escribe `ActivePiece`, etc.). `render.c` e `input.c` son de solo lectura sobre `GameState` (`input.c` ni siquiera lo lee — produce `InputIntent` aparte, que `piece.c` consume).

## 4. Estructuras de datos principales

| Struct | Campos (tipo — significado) |
| --- | --- |
| `BoardState` | `board[22][10]` `u8` — 20 filas visibles + 2 de buffer superior para spawn (reducido de las 40 filas de Apotris: sin big-mode ni animación diferida, el margen de 2 filas es proporcional al de Apotris para el chequeo de spawn/game-over) |
| `ActivePiece` | `type` `u8` (0-6) · `x,y` `u8` · referencia a la forma rotación-0 en `piece_data` (no se copia) |
| `PieceQueue` | `bag[7]` `u8` + `bagCount` `u8` · `queue[N]` `u8` (N=1 mínimo funcional; recomendado N=2 si se quiere mostrar "next" aunque no sea requisito explícito del MVP) |
| `GravityState` | `level` `u8` · `speedFixed` `u16` (Q8.8, de la tabla convertida offline) · `speedCounter` `u16` (acumulador Q8.8) |
| `LinesToClear` | `rows[4]` `u8` · `count` `u8` (máx. 4 líneas simultáneas) |
| `InputIntent` (no vive en `GameState`) | `left,right,down` — flags de intención, producidos por `input.c` cada frame |

## 5. Frame loop determinista

1. Leer input (PVSnesLib ya lo captura en el VBlank-ISR; `input.c` traduce `pad_keys`/`pad_keysdown` a `InputIntent` al inicio del frame lógico).
2. Si no hay pieza activa → `piece_spawn()` (consume `queue`, repone vía `queue_next()`).
3. Movimiento horizontal (DAS/ARR) usando `board_is_cell_occupied` para colisión.
4. Gravedad: acumulador Q8.8 (`speedCounter += speedFixed; n = speedCounter >> 8; speedCounter &= 0xFF;`) → `n` pasos de caída con chequeo de colisión.
5. Recalcular fila de aterrizaje (equivalente a `lowest()`) para el lock-timer.
6. Lock-delay: decrementar si la pieza está apoyada; en 0 → lock (escribir en `board`, chequear derrota, disparar line-clear).
7. Line clear: detectar filas completas → colapsar de inmediato (sin el delay de animación de Apotris, ya descartado en el deep-dive) → `piece_spawn()`.
8. `render_sync(GameState*)` — una sola sincronización de VRAM/OAM al final del frame, sobre el estado ya resuelto.
9. `WaitForVBlank()`.

Determinismo: todo en enteros (Q8.8, sin floats); un único `rand()` por pieza extraída del bag; sin timing variable.

## 6. Input PVSnesLib

Confirmado (`include/snes/input.h`): `pad_keys[5]`/`pad_keysdown[5]`, poblados automáticamente por el VBlank-ISR (sin polling manual); bits `KEY_LEFT`, `KEY_RIGHT`, `KEY_DOWN` (entre otros) vía `KEYPAD_BITS`. `input.c` lee `pad_keys[0]`/`pad_keysdown[0]` y produce `InputIntent{left,right,down}` — así ningún otro módulo depende del nombre exacto de las constantes de PVSnesLib.

## 7. Separación gameplay/render

`board.c`, `piece.c`, `piece_data.c`, `queue.c` no incluyen ningún header de video de PVSnesLib (`background.h`, `oam.h`) ni de input directamente. `render.c` es el único módulo que los incluye y el único que escribe VRAM/OAM. `game_state.h` solo depende de `u8`/`u16` (`snestypes.h`, ya neutral de plataforma).

## 8. Estrategia mínima de tilemap/VRAM

- **Playfield (board):** 1 capa de background (confirmado en el ejemplo hello-world de PVSnesLib: `setMode(BG_MODE1, 0)`), inicializada una vez con `bgInitTileSet`/`bgInitMapSet` (funciones confirmadas en `background.h`). El tilemap se actualiza **solo** en eventos de baja frecuencia (lock, line-clear) — nunca por movimiento/gravedad.
  - **Confirmado** (ver `_bmad-output/implementation-artifacts/investigations/pvsneslib-tilemap-runtime-update-investigation.md`): `background.h` no expone escritura de celda individual — ninguna de sus 12 funciones lo hace. El patrón real (ejemplo `games/breakout/breakout.c`, equivalente en escala: romper un ladrillo) es mantener un **buffer espejo del tilemap en WRAM**, mutarlo directamente (`buffer[i] = tile`) al hacer lock/line-clear, y transferirlo con `dmaCopyVram(u8 *source, u16 address, u16 size)` (confirmada en `dma.h`) inmediatamente después de `WaitForVBlank()`.
  - **Decisión:** transferir el **tilemap completo del playfield** (no celda-a-celda) en cada lock/line-clear — el playfield (10×20, formato SC_32x32) pesa ~400 bytes, un orden de magnitud menos que el bloque de 2048 bytes que breakout transfiere por cada evento equivalente. Trackear regiones "dirty" sería complejidad sin beneficio medible a este tamaño.
- **Pieza activa:** hasta 4 sprites OAM (`oamSet`/`oamSetXY`, confirmados en `oam.h`) — una posición por celda ocupada de la forma rotación-0. Reposicionar sprites cada frame que la pieza se mueve (barato, sin tocar el tilemap). Al hacer lock: "hornear" la pieza en el tilemap del playfield (una escritura por celda, evento infrecuente) y ocultar los sprites (`oamSetVisible`) hasta el próximo spawn.
- Esto evita reescribir el tilemap completo cada frame por gravedad/movimiento — solo los sprites se mueven en el hot path.

## 9. Inicialización mínima de ROM/PVSnesLib

Basada en la secuencia confirmada del ejemplo hello-world:

1. `setMode(BG_MODE1, 0)` (o el modo de color que corresponda a los tiles del playfield).
2. `bgInitTileSet` + `bgInitMapSet` para el BG del playfield (una vez, al boot).
3. `oamInitGfxSet` para los gráficos de sprites de las piezas.
4. `bgSetDisable(...)` para los BGs no usados.
5. `setScreenOn()`.
6. Entrar al bucle principal: `while(1) { <frame loop sección 5>; WaitForVBlank(); }`.

## 10. Orden recomendado de implementación

1. `game_state.h` — structs sin lógica, define el contrato entre módulos.
2. `board.c` — tablero + colisión + detección/colapso de líneas (testeable sin hardware de video).
3. `piece_data.c` — tablas reutilizadas de Apotris (forma rotación-0, gravedad Q8.8).
4. `piece.c` — spawn, movimiento horizontal, gravedad, lock, usando `board.c`.
5. `queue.c` — bag/`rand()` integrado a `piece_spawn`.
6. Frame loop lógico en `main.c` **sin render** — validar reglas con la consola de texto de PVSnesLib (`consoleInitText`, confirmada en el hello-world) antes de tocar sprites/tilemap.
7. `input.c` — reemplaza cualquier input simulado de depuración por pads reales.
8. `render.c` — tilemap del playfield + sprites de la pieza activa, al final, una vez la lógica ya está validada sin gráficos.
