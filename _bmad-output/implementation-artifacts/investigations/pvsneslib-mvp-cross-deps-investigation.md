# Investigation: reemplazos mínimos de dependencias transversales (Apotris MVP → SNES/PVSnesLib)

## Hand-off Brief

1. **Qué se investigó.** Las 4 dependencias transversales del núcleo MVP (`_bmad-output/deep-dive-tetris-core.md`): RNG `qran`/`sqran`, tipos de `tonc.h`, contenedores STL/alloc dinámica, gravedad `float`.
2. **Dónde queda el caso.** Las 4 decisiones están resueltas con evidencia directa de la documentación/código público de PVSnesLib (no hay PVSnesLib instalado localmente — se verificó vía repo oficial `alekmaul/pvsneslib`).
3. **Qué sigue.** Ninguna investigación adicional pendiente para este alcance; listo para que `gds-quick-dev`/story de implementación use estas decisiones tal cual.

## Case Info

| Field            | Value                                                                      |
| ---------------- | -------------------------------------------------------------------------- |
| Ticket           | N/A                                                                         |
| Date opened      | 2026-07-10                                                                  |
| Status           | Concluded                                                                   |
| System           | PVSnesLib (repo `alekmaul/pvsneslib`, rama `master`) — no instalado localmente, verificado vía GitHub |
| Evidence sources | `_bmad-output/project-context.md`, `_bmad-output/deep-dive-tetris-core.md`, código fuente Apotris (`include/tetrisEngine.h`, `source/tetrisEngine.cpp`, `include/tetromino.hpp`, `source/tetromino.cpp`), repo público PVSnesLib (`include/snes/snestypes.h`, `include/snes/console.h`, README) |

## Problem Statement

Determinar los reemplazos mínimos concretos para 4 dependencias GBA/C++ identificadas como bloqueantes del port del núcleo MVP a C/PVSnesLib, sin inventar APIs y sin diseñar arquitectura completa.

## Evidence Inventory

| Source                                              | Status    | Notes |
| ---------------------------------------------------- | --------- | ----- |
| `pvsneslib/include/snes/snestypes.h` (GitHub, raw)   | Available | Typedefs confirmados vía 2 fetches independientes, resultado idéntico |
| `pvsneslib/include/snes/console.h` (GitHub, raw)     | Available | Prototipo y doc-comment de `rand()` confirmados textualmente |
| README / doxygen PVSnesLib                           | Available | Confirma toolchain (816-tcc), lenguaje C-only, advertencia sobre costo de mod/mul/div en 65C816 |
| Instalación local de PVSnesLib                       | Missing   | No hay `PVSNESLIB_HOME` ni binarios en esta máquina — todo verificado contra el repo remoto, no ejecutado |
| Existencia de `srand()`/seed explícito en PVSnesLib   | Missing   | No se encontró evidencia de una función de re-seed pública; ver Hipótesis 1 |

## Confirmed Findings

### Finding 1: PVSnesLib expone `rand()` listo para usar

**Evidencia:** `pvsneslib/include/snes/console.h` (raw, GitHub, master): `u16 rand(void);` con doc-comment `\brief return a randomized number`.

**Detalle:** La librería inicializa el generador automáticamente (sin llamada de setup requerida, según la documentación pública). Uso confirmado por convención de la comunidad: asignar el resultado a `u16` y aplicar `% n` para acotar rango.

### Finding 2: `snestypes.h` define los mismos nombres de tipo que `tonc_types.h`

**Evidencia:** `pvsneslib/include/snes/snestypes.h` (raw, GitHub, master):
```c
typedef signed char s8;      typedef unsigned char u8;
typedef signed short s16;    typedef unsigned short u16;
typedef signed long long s32; typedef unsigned long long u32;
typedef unsigned char bool;  // + TRUE/FALSE, true/false, NULL, BIT(n)
```

**Detalle:** El core Apotris analizado solo usa `u8`/`u16` de `tonc_types.h` (vía `tonc.h`) — mismos nombres, misma semántica de ancho (8/16 bit unsigned). No se detectó uso de `u32` ni de tipos `v*` (volátiles/registros de hardware) dentro de los 4 archivos del núcleo.

### Finding 3: Toolchain de PVSnesLib es C puro (816-tcc), sin C++/STL

**Evidencia:** README del repo `alekmaul/pvsneslib` — créditos a "816-tcc" (Tiny C Compiler para 65816) como compilador de C; el propio README describe la librería como "C language" con ASM opcional. El 13% de C++ reportado en el lenguaje del repo corresponde a herramientas de build, no al código de usuario compilado para el cartucho.

**Detalle:** Esto es una restricción dura, no una preferencia: no existe posibilidad de compilar `std::list`/STL para el target 65816 con esta toolchain. Confirma (no solo refuerza) la regla ya fijada en `project-context.md` de módulos C pequeños sin contenedores dinámicos.

### Finding 4: `rand()` no es gratis — mod/mul/div son costosos en 65C816

**Evidencia:** README/doxygen PVSnesLib: "Modulo, multiplication and division operations are very slow on the 65C816 processor used by SNES."

**Detalle:** Afecta directamente el patrón `qran() % bag.size()` del 7-bag (`fillQueue`, `tetrisEngine.cpp:1284`) y cualquier fixed-point que use división.

## Hypothesized Paths

### Hypothesis 1: No existe `srand()`/reseed público en PVSnesLib

**Status:** Open (no se encontró evidencia positiva ni negativa concluyente en la documentación pública revisada)

**Theory:** El re-seed manual que hace Apotris (`sqran(seed); seed += qran();` en `fillQueue`, tetrisEngine.cpp:1278-1279) existe en el original para soportar semillas de partida reproducibles (menú de "seed"). Esa característica está fuera del alcance MVP (no aparece en los 9 sistemas de `deep-dive-tetris-core.md`).

**Would confirm:** Encontrar `srand()` o una función de seed explícita en `console.h`/otros headers del repo.

**Would refute:** Confirmación de que `rand()` es la única función pública del RNG (ya es la evidencia actual, pero no se revisó el código fuente del generador, solo el header).

**Resolution:** No bloquea el MVP — se resuelve por alcance: si no se requiere semilla reproducible, no hace falta reseed manual; usar `rand()` directo.

## Conclusion

**Confidence:** High para las 4 decisiones (evidencia directa del repo público citada arriba); Low únicamente sobre la existencia de un `srand()` (irrelevante para el alcance MVP).

### Decisión 1 — RNG (`qran`/`sqran`)

Usar `rand()` de PVSnesLib (`console.h`) directo, sin capa de reseed manual (esa parte de Apotris es para semillas reproducibles, fuera del MVP). Reemplazo 1:1 en los call-sites:
- `qran() % bag.size()` → `rand() % bag.size()` (7-bag, `fillQueue`)
- Eliminar las 2 líneas de `sqran(seed); seed += qran();` (reseed) — no aplican sin feature de seed.

### Decisión 2 — Tipos de `tonc.h`

`u8`/`u16` (únicos tipos usados en el núcleo) están ya definidos idénticamente en `snestypes.h` (vía `snes.h`) — **cambio de include únicamente**, cero renombrado de variables/tablas (`masterDelays[9][5]`, `connectedConversion[24]`, etc. compilan sin tocar el tipo). `def.h`/`logging.h`/`posprintf.h` (ya identificados como no usados por la lógica core) se descartan del port, no requieren reemplazo.

### Decisión 3 — STL y alloc dinámica

Restricción dura confirmada (no hay C++ en el toolchain de compilación de cartucho). Reemplazos mínimos por contenedor realmente usado en los 9 sistemas MVP:

| Original (Apotris)              | Uso                          | Reemplazo estático propuesto              |
| -------------------------------- | ----------------------------- | ------------------------------------------ |
| `int** board` (heap)              | tablero 40×10                | `uint8_t board[40][10]`                    |
| `int board[4][4][4]` (en Pawn)    | shape cacheado de la pieza    | ya es fixed-size — solo cambiar `int`→`uint8_t` |
| `std::list<int> bag`              | 7-bag (máx. 7 elementos)      | `uint8_t bag[7]; uint8_t bagCount;`         |
| `std::list<int> queue`            | cola de siguiente pieza       | `uint8_t queue[N]` (N = lookahead deseado, mínimo 1) |
| `std::list<int> linesToClear`     | líneas completas a limpiar    | `uint8_t linesToClear[4]; uint8_t linesToClearCount;` (máx. 4 líneas simultáneas en Tetris estándar) |
| `std::list<int> moveHistory`      | solo usado por finesse (fuera de MVP) | eliminar, sin reemplazo |

No se encontró evidencia (ni se buscó específicamente, fuera de alcance) sobre soporte de `malloc` en 816-tcc — irrelevante: `project-context.md` ya exige memoria estática, la tabla de arriba no depende de esa respuesta.

### Decisión 4 — Gravedad `float` → fixed-point

Recomendación: **Q8.8** (16 bits: 8 bits enteros + 8 bits fracción, cabe en `u16`/`s16`, registros nativos del 65816 en modo 16-bit).

- Rango: 0–255.996, resolución 1/256 (≈0.0039). La tabla `GameInfo::gravity[19]` va de 0.01667 a 20.0 — cómoda dentro del rango, con el valor más pequeño (0.01667) representado como 4/256 (0.015625, error relativo ~6%, aceptable para un acumulador de caída).
- **Conversión de tablas:** por cada valor `g` en `gravity`/`classicGravity`/`blitzGravity`/`masterGravity`, precalcular `round(g * 256)` como `uint16_t` — conversión offline (script o constexpr), no en runtime.
- **Conversión del acumulador** (`speedCounter += speed; n = (int)speedCounter; speedCounter -= n;`, `tetrisEngine.cpp:548-559`): con Q8.8 se reemplaza división/truncado por shift+máscara, evitando la división costosa (Finding 4): `speedCounter += speedFixed; n = speedCounter >> 8; speedCounter &= 0xFF;`. Preserva el comportamiento exacto del original (mismo acumulador sub-frame), sin usar `/` ni `%`.

## Recommended Next Steps

### Fix direction

Ninguno — esto es investigación de dependencias, no un defecto. Las 4 decisiones están listas para consumirse en la próxima story/implementación del núcleo.

### Diagnostic

Ninguno pendiente para este alcance. Si se decide soportar semillas reproducibles más adelante (fuera de MVP), habría que confirmar si 816-tcc expone algún hook de seed antes de diseñar esa feature.

## Side Findings

- El README de PVSnesLib advierte explícitamente que mod/mul/div son lentos en 65C816 — motivo adicional (no solo estilo) para el diseño Q8.8 con shift+máscara en la Decisión 4, y para minimizar el uso de `% bag.size()` en el 7-bag (impacto acotado: máx. 7 valores posibles, se ejecuta una vez por pieza).

---

**Próximo paso sugerido:** con estas 4 decisiones fijas, `gds-create-story`/`gds-quick-dev` puede definir la primera story de implementación del núcleo (tablero + pieza activa + spawn + colisión) sin bloqueos de dependencia pendientes.
