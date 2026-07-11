# Investigation: actualización runtime del tilemap del playfield (PVSnesLib)

## Hand-off Brief

1. **Qué se investigó.** El cabo abierto de `_bmad-output/game-architecture.md` §8: cómo actualizar celdas del BG tilemap del playfield en runtime (lock/line-clear) con PVSnesLib.
2. **Dónde queda el caso.** Resuelto con evidencia directa: no hay función de escritura de celda individual en la API; el patrón real usado en un juego shipeado con PVSnesLib es buffer WRAM + `dmaCopyVram`.
3. **Qué sigue.** Ninguna investigación adicional; `game-architecture.md` §8 actualizado con la decisión confirmada.

## Case Info

| Field            | Value                                                                      |
| ---------------- | -------------------------------------------------------------------------- |
| Ticket           | N/A                                                                         |
| Date opened      | 2026-07-10                                                                  |
| Status           | Concluded                                                                   |
| System           | PVSnesLib (repo `alekmaul/pvsneslib`, rama `master`) — verificado vía GitHub, no instalado localmente |
| Evidence sources | `_bmad-output/game-architecture.md` (§8), `include/snes/background.h` (raw), `include/snes/dma.h` (raw), `snes-examples/games/breakout/breakout.c` (raw) |

## Problem Statement

Determinar la API real de PVSnesLib para actualizar celdas/regiones pequeñas de un BG tilemap ya inicializado durante gameplay, y la estrategia mínima para el playfield de Tetris (cambia solo en lock/line-clear).

## Evidence Inventory

| Source                                                        | Status    | Notes |
| -------------------------------------------------------------- | --------- | ----- |
| `pvsneslib/include/snes/background.h` (raw, GitHub)            | Available | Lista completa de funciones de background: todas son de init/scroll/enable — ninguna de escritura de celda individual |
| `pvsneslib/include/snes/dma.h` (raw, GitHub)                    | Available | `dmaCopyVram(u8 *source, u16 address, u16 size)` confirmada — copia un rango de bytes desde WRAM a una dirección VRAM |
| `snes-examples/games/breakout/breakout.c` (raw, GitHub)         | Available | Ejemplo real: mutación de buffer WRAM (`blockmap[...] = 0`) + `dmaCopyVram(...)` inmediatamente después de `WaitForVBlank()` |

## Confirmed Findings

### Finding 1: `background.h` no expone escritura de celda individual

**Evidencia:** `pvsneslib/include/snes/background.h` (raw, GitHub, master) — 12 funciones listadas (`bgSetGfxPtr`, `bgSetMapPtr`, `bgInitTileSet*`, `bgInitMapSet`, `bgSetScroll`, `bgSetEnable/Disable[Sub]`, `bgSetWindowsRegions`). Ninguna escribe una celda o región del tilemap ya cargado.

**Detalle:** Confirma (no solo refuerza) lo ya marcado como abierto en `game-architecture.md`: no existe una función de alto nivel para esto en la librería.

### Finding 2: patrón real confirmado — buffer WRAM + `dmaCopyVram`, durante VBlank

**Evidencia:** `snes-examples/games/breakout/breakout.c` — al destruir un ladrillo: `blockmap[0x42+b]=0; blockmap[0x43+b]=0; backmap[0x63+b]-=0x400; backmap[0x64+b]-=0x400;` seguido de `dmaCopyVram((u8*)blockmap, 0x0000, 0x800); dmaCopyVram((u8*)backmap, 0x0400, 0x800);`, ejecutado inmediatamente después de `WaitForVBlank()`.

**Detalle:** `dma.h` confirma la firma: `void dmaCopyVram(u8 *source, u16 address, u16 size);` — copia `size` bytes desde un buffer WRAM a una dirección VRAM (canal 0 de DMA). El propio header no documenta una restricción explícita de "solo en VBlank", pero el ejemplo real la aplica como práctica (evita corrupción visual con la PPU activa).

## Deduced Conclusions

### Deducción 1: transferir el playfield completo es la estrategia correcta para Tetris, no celda-a-celda

**Basado en:** Finding 1 (no hay escritura celda-a-celda) + Finding 2 (breakout transfiere 0x800 bytes = 2048 bytes por evento, no una celda).

**Razonamiento:** El playfield de Tetris (10×20 visibles) en formato de mapa estándar SC_32x32 (2 bytes/entrada) pesa ~400 bytes — un orden de magnitud menor que el bloque de 2048 bytes que breakout transfiere por cada ladrillo roto. El evento que dispara la actualización (lock o line-clear) es tan infrecuente como "romper un ladrillo". No hay motivo para trackear regiones "dirty" — sería complejidad sin beneficio medible al tamaño de este playfield.

**Conclusión:** Mantener un buffer WRAM espejo del tilemap del playfield, mutarlo en `board.c`/lock/line-clear, y hacer un único `dmaCopyVram()` del playfield completo, inmediatamente después de `WaitForVBlank()` — igual que el patrón confirmado en breakout.

## Conclusion

**Confidence:** High. Evidencia directa de dos fuentes primarias (`background.h`, `dma.h`) más un ejemplo de juego real y equivalente en escala (breakout) que resuelve exactamente el mismo problema (actualizar región pequeña de tilemap tras evento de gameplay infrecuente).

- No existe API de PVSnesLib para escritura celda-a-celda — confirmado, no hipotetizado.
- La solución real usada en la práctica: buffer WRAM + `dmaCopyVram`, durante/tras VBlank.
- Para el playfield de Tetris: transferir el tilemap completo (no por celda) en cada lock/line-clear — más simple, determinista, y barato en relación al precedente real (breakout).

## Recommended Next Steps

### Fix direction

N/A — decisión de arquitectura, no defecto. Ya aplicada en `game-architecture.md` §8.

### Diagnostic

Ninguno pendiente.

## Side Findings

- `dmaCopyVram` no documenta explícitamente la restricción de VBlank en su propio header — la evidencia de que debe ejecutarse ahí viene del ejemplo real (breakout), no de la documentación de la función. Si en implementación se observa parpadeo/corrupción al transferir fuera de VBlank, es la primera hipótesis a revisar.

---

**Próximo paso sugerido:** ninguna investigación adicional necesaria — continuar con `gds-create-story`/`gds-quick-dev` sobre el orden de implementación ya definido en `game-architecture.md` §10.
