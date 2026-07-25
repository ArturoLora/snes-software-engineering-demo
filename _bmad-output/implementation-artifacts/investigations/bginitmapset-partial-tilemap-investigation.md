# Investigation: ¿`bgInitMapSet()` acepta un tilemap menor a 32×32 con `SC_32x32`?

## Hand-off Brief

1. **Qué pasó.** Story 4.4 reemplazó `playfield.png` (10×20 tiles) por arte con borde visible; en Ares el playfield se ve como una "cuadrícula" que cubre toda la pantalla en vez de solo el área 10×20, y el borde de acento no se distingue.
2. **Dónde está el caso.** Pregunta puntual respondida con evidencia local (código propio + ejemplo oficial de PVSnesLib). Confianza: Media (Deducido, no Confirmado al 100% — no se leyó el algoritmo interno de `gfx4snes`/`bgInitMapSet`, ambos son binarios sin fuente instalada localmente).
3. **Qué sigue.** Ensanchar el canvas de `playfield.png` a 32 tiles de ancho (256px), dejando el contenido real en la esquina superior izquierda (10×20) y el resto en blanco/fondo — evita el desalineamiento de stride sin tocar `main.c`.

## Case Info

| Field            | Value                                                                      |
| ---------------- | -------------------------------------------------------------------------- |
| Ticket           | N/A (pregunta puntual de Arturo, post Story 4.4)                          |
| Date opened      | 2026-07-12                                                                 |
| Status           | Concluded                                                                  |
| System           | PVSnesLib (instalación local `~/Tools/pvsneslib`), gfx4snes 2.0.0          |
| Evidence sources | Código propio (`snes/source/main.c`, `snes/Makefile`, `snes/data.asm`), ejemplo oficial `snes-examples/graphics/Backgrounds/Mode1Png/Mode1.c` |

## Problem Statement

¿`bgInitMapSet(bgNumber, mapSource, mapSize, SC_32x32, address)` acepta un `mapSize` menor al nametable completo (32×32 = 1024 tiles / 2048 bytes), o es obligatorio generar siempre un tilemap completo de 32×32?

## Evidence Inventory

| Source   | Status                          | Notes     |
| -------- | ------------------------------- | --------- |
| `snes/source/main.c:108-109` | Available | Nuestra llamada real a `bgInitMapSet` |
| `snes/data.asm`, `snes/playfield.map` | Available | Tamaño real generado: 400 bytes (200 tiles) |
| `snes-examples/graphics/Backgrounds/Mode1Png/Mode1.c` | Available | Único ejemplo oficial local que usa PNG + `-m` (mismas flags gfx4snes que nuestro Makefile) |
| Fuente de `bgInitMapSet`/`gfx4snes` (algoritmo interno) | Missing | Librería/herramienta instaladas como binarios, sin `.c`/`.s` fuente en `~/Tools/pvsneslib` |

## Confirmed Findings

### Finding 1: Nuestro código pasa el tamaño real generado, no 2048 fijo

**Evidencia:** `snes/source/main.c:108-109`:
```c
bgInitMapSet(1, (u8 *)&playfieldmap, (u16)(&playfieldmap_end - &playfieldmap), SC_32x32, 0x7000);
```
`playfield.map` = 400 bytes (verificado con `ls -la`) = 200 tiles = exactamente 10×20 (`playfield.png` = 80×160px).

### Finding 2: El ejemplo oficial hace exactamente lo mismo (tamaño variable, no 2048 fijo)

**Evidencia:** `Mode1Png/Mode1.c:22`:
```c
bgInitMapSet(0, &map, (&map_end - &map), SC_32x32, 0x0000);
```
Mismo patrón: `mapSize` calculado del símbolo generado, no un literal `2048`. Su Makefile usa las mismas flags `gfx4snes -s 8 -o 16 -u 16 -e 0 -p -m` que el nuestro (`Backgrounds/Mode1Png/Makefile:20`).

### Finding 3: La diferencia estructural clave es el ANCHO en tiles, no el alto

**Evidencia:** `pvsneslib.png` (asset del ejemplo oficial) = 256×224px = **32×28 tiles** (ancho=32, alto=28 — menor a 32 filas, y aun así funciona, según el ejemplo oficial). Nuestro `playfield.png` = 80×160px = **10×20 tiles** (ancho=10, alto=20).

## Deduced Conclusions

### Deducción 1: Un mapa más chico que 32×32 es válido — pero solo si el ANCHO coincide con el stride de `SC_32x32`

**Basado en:** Finding 1, 2, 3.

**Razonamiento:** El ejemplo oficial prueba que un `mapSize` parcial (no 2048) es el uso normal y soportado — su imagen tiene menos FILAS que 32 (28, no 32) y funciona correctamente. La diferencia con nuestro caso no es "tamaño total menor", es que nuestra imagen tiene menos COLUMNAS que 32 (10, no 32). `gfx4snes -m` genera el `.map` empaquetado fila por fila según el ANCHO EN TILES de la imagen fuente (10 en nuestro caso, 32 en el oficial) — sin relleno hasta 32 columnas. `SC_32x32` le dice al hardware que cada fila del nametable en VRAM mide 32 tiles. Si los datos vienen empaquetados con stride 10 pero el hardware los direcciona con stride 32, las filas se desalinean: el tile en posición lógica (fila 1, columna 0) del PNG termina escribiéndose en la posición VRAM (fila 0, columna 10) en vez de (fila 1, columna 0) — un corrimiento diagonal que dispersa el contenido real por buena parte del nametable de 1024 tiles, y dejaría cientos de entradas sin inicializar apuntando a índices de tile fuera de los ~9 tiles reales generados (de ahí la apariencia de "cuadrícula" cubriendo toda la pantalla, con contenido no controlado — coherente con lo reportado, aunque el tono "rojo" específico no se puede confirmar solo con este análisis).

**Conclusión:** Respuesta a la pregunta única — **sí es válido cargar un `mapSize` menor a 2048 bytes** (el ejemplo oficial lo hace), **pero el ancho en tiles de la imagen fuente debe ser 32** para que el stride de `-m`/`gfx4snes` coincida con `SC_32x32`. Un ancho de 10 (nuestro caso) rompe esa alineación.

## Hypothesized Paths

### Hipótesis 1: El desalineamiento de stride explica el patrón "repetido"/"toda la pantalla"

**Status:** Open (no Confirmada — requeriría leer el algoritmo de empaquetado de `gfx4snes -m` o volcar el contenido real de VRAM en Ares para confirmar byte a byte)

**Teoría:** Ver Deducción 1.

**Indicadores a favor:** El placeholder anterior (grid uniforme, todos los tiles visualmente idénticos) habría ocultado este mismo desalineamiento sin que se notara — coherente con que el problema "apareciera" recién al cambiar a un arte con tiles distintos (borde vs. interior).

**Confirmaría:** Volcar el nametable real de VRAM en Ares (0x7000 en adelante) y compararlo byte a byte contra `playfield.map`, o leer el algoritmo de empaquetado de `gfx4snes`.

**Refutaría:** Si el volcado de VRAM muestra los 200 tiles exactamente en las primeras 10 columnas × 20 filas sin corrimiento, la teoría de stride queda refutada y el origen sería otro (ROM vieja cacheada en Ares, paleta/CGRAM sin inicializar, etc.).

**Resolution:** N/A — Open, no se ejecutó el volcado de VRAM (fuera del alcance de esta pregunta puntual).

## Missing Evidence

| Gap              | Impact                               | How to Obtain   |
| ---------------- | ------------------------------------ | --------------- |
| Algoritmo de empaquetado de `gfx4snes -m` (fuente no instalada localmente) | No permite Confirmar el mecanismo exacto del desalineamiento, solo Deducirlo por analogía con el ejemplo oficial | Volcado de VRAM en Ares, o documentación oficial de gfx4snes (fuera de alcance de esta pregunta: "no investigar otros temas") |
| Origen exacto del tono "rojo" reportado | No se puede atribuir con certeza a este mecanismo | Volcado de VRAM/CGRAM en Ares |

## Conclusion

**Confidence:** Medium (Deducido de evidencia Confirmada — el patrón del ejemplo oficial y el cálculo de `mapSize` — pero sin leer el algoritmo interno de `gfx4snes`/`bgInitMapSet`, que son binarios sin fuente local).

Un `mapSize` menor al nametable completo **es válido y es el uso estándar** de `bgInitMapSet()` (el ejemplo oficial `Mode1Png` lo prueba). El requisito real no es "generar 32×32 completo" sino que **la imagen fuente debe medir 32 tiles de ancho** para que el stride del `.map` generado por `gfx4snes -m` coincida con el `SC_32x32` declarado. Nuestro `playfield.png` mide 10 tiles de ancho — ahí está el desalineamiento más probable.

## Recommended Next Steps

### Fix direction

Ensanchar el canvas de `playfield.png` a 256×160px (32 tiles de ancho × 20 de alto), dejando el playfield real (10×20) en la esquina superior izquierda y el resto (columnas 10-31) en un tile de fondo/vacío. Esto no requiere ningún cambio en `main.c` (mismos punteros, mismo `SC_32x32`, misma dirección VRAM) — solo el asset y su Makefile target (`playfield.pic`) se reconvierten solos con `make`.

### Diagnostic

Si tras ese cambio la cuadrícula sigue cubriendo toda la pantalla, el siguiente paso sería volcar el nametable real desde Ares (memory viewer, VRAM $7000+) para comparar contra `playfield.map` byte a byte — eso confirmaría o refutaría la Hipótesis 1 de forma directa.

## Reproduction Plan

1. Regenerar `playfield.png` a 256×160px (32×20 tiles), contenido real en cols 0-9, resto relleno.
2. `make`.
3. Ejecutar en Ares — confirmar que el playfield ocupa solo el rectángulo 10×20 esperado y que el resto de la pantalla queda en blanco/fondo.

## Side Findings

- El tono "rojo" reportado no tiene explicación en la paleta actual de `playfield.png` (`(12,14,28)` + `(96,104,128)`, sin rojo) — es coherente con que sean tiles fuera de rango/VRAM no inicializada, pero no se puede confirmar sin volcado de VRAM (fuera de alcance de esta pregunta puntual).
