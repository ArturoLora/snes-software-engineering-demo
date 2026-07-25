# PoC: lectura de memoria SNES desde BizHawk vía Lua

Objetivo: demostrar que BizHawk 2.11.1 puede leer el estado de la ROM de Apotris
SNES frame a frame desde Lua. **No** es el harness, no automatiza nada, no toca
el código del juego.

## Archivos

| Archivo | Qué es |
|---|---|
| `poc_read_memory.lua` | El script del PoC. Único entregable ejecutable. |
| `poc_read_memory.log` | Se genera al correr. Copia de la salida de consola, para revisarla después. |

## Dónde va el script

Puede ir en cualquier ruta legible; BizHawk abre scripts por ruta absoluta. Aquí
vive en el repo, fuera de `snes/`, para no mezclarlo con el código del juego:

```
tools/lua/poc_read_memory.lua
```

No hace falta copiarlo a `tools/BizHawk-2.11.1-linux-x64/Lua/`.

## Cómo ejecutarlo

1. Arrancar BizHawk:

   ```bash
   cd /home/arturo/Projects/apotris-snes/tools/BizHawk-2.11.1-linux-x64
   ./EmuHawkMono.sh
   ```

2. Cargar la ROM: **File → Open ROM** → `/home/arturo/Projects/apotris-snes/snes/apotris.sfc`

3. Abrir la consola Lua: **Tools → Lua Console**

4. En la ventana Lua Console: **Script → Open Script...** →
   `/home/arturo/Projects/apotris-snes/tools/lua/poc_read_memory.lua`

   El script arranca solo al abrirse. Si se editó el archivo, **Script → Reload**.

5. Asegurarse de que la emulación no está pausada (`Pause` = tecla por defecto).

### Vía línea de comandos (equivalente, y automatizable)

`--lua` **sí** existe en esta build. Todo lo anterior en un solo comando:

```bash
cd /home/arturo/Projects/apotris-snes/tools/BizHawk-2.11.1-linux-x64
./EmuHawkMono.sh \
  --lua=/home/arturo/Projects/apotris-snes/tools/lua/poc_read_memory.lua \
  --luaconsole \
  /home/arturo/Projects/apotris-snes/snes/apotris.sfc
```

O directamente el harness, que además valida el resultado:

```bash
DISPLAY=:0 python3 tools/harness/harness.py
```

Ver `tools/harness/README.md` para el detalle de los flags soportados y las
limitaciones del emulador.

## Qué se lee, y de dónde salieron las direcciones

Las direcciones no son adivinadas. Vienen de `snes/apotris.sym` (tabla de
símbolos que emite `wlalink` al enlazar) cruzada con `snes/source/game_state.h`:

```
007e2000  pad0                       <- estado del pad (PVSnesLib), u16
007e2002  tccs_source/main.asm_gs    <- la variable global `gs` de main.c
```

`GameState` es todo `u8`, así que no hay padding y los offsets son directos:

| Campo | Offset en `gs` | Dirección de bus |
|---|---|---|
| `board[22][10]` | 0 … 219 | `$7E2002` … `$7E20DD` |
| `piece.type` | 220 | `$7E20DE` |
| `piece.rotation` | 221 | `$7E20DF` |
| `piece.x` (s8) | 222 | `$7E20E0` |
| `piece.y` (s8) | 223 | `$7E20E1` |
| `lines.rows[4]` | 224 … 227 | `$7E20E2` … `$7E20E5` |
| `lines.count` | 228 | `$7E20E6` |

Comprobación del layout: el símbolo siguiente en el `.sym` es
`007e20e7 tccs_libc_c.asm_msys`, y `0x7E2002 + 229 = 0x7E20E7`. Encaja exacto →
el tamaño y los offsets asumidos son correctos, sin padding.

## Dominio de memoria

Dominios reales de este core SNES, medidos en ejecución:

```
VRAM (65536 bytes)
CARTROM (262144 bytes)
Waterbox PageData (6869 bytes)
WRAM (131072 bytes)
```

**No hay `System Bus`** en este core. `WRAM` es el correcto.

El script **no asume** el dominio. Al arrancar llama a
`memory.getmemorydomainlist()`, imprime todos los dominios del core cargado con
su tamaño, y elige:

1. `WRAM` si existe — son los 128 KiB de RAM de trabajo del SNES, donde el
   offset 0 del dominio equivale a la dirección de bus `$7E0000`. El script
   resta esa base: `$7E20E1` → offset `0x20E1`.
2. Si no, `System Bus`, donde la dirección se usa tal cual (`$7E20E1`).
3. Si no hay ninguno de los dos, aborta con mensaje explícito.

Así el PoC responde por sí mismo cuál es el dominio correcto, en vez de
depender de documentación externa.

Ojo: `Lua/_docs_luacats/memory.d.lua` de esta misma build dice que
`getmemorydomainlist()` devuelve *"un string delimitado por saltos de línea"*.
En 2.11.1 devuelve una **tabla**. El script acepta ambas formas.

## Cómo verificar que realmente está leyendo memoria

Tres señales independientes:

1. **Cabecera en la consola Lua.** Debe listar los dominios reales del core y
   confirmar el elegido, p. ej.:

   ```
   Dominios de memoria del core actual:
     - WRAM (131072 bytes)
     ...
   Sistema ....... SNES
   Dominio usado . WRAM
   pad0 en bus $7E2000 -> offset 0x2000 del dominio
   gs   en bus $7E2002 -> offset 0x2002 del dominio
   ```

   Si el dominio no existiera, el script fallaría en voz alta en lugar de
   imprimir ceros silenciosos.

2. **Reacción al input.** Manteniendo un botón del pad, la línea `CHANGE` debe
   mostrar `pad=` distinto de `0000`, y volver a `0000` al soltar. Eso prueba
   que se está leyendo RAM viva y no una copia estática.

3. **Reacción al estado del juego.** `piece[x=… y=…]` debe cambiar cuando la
   pieza se mueve (input horizontal o gravedad automática). Se puede
   contrastar contra **Tools → RAM Watch** añadiendo a mano `$7E20E0` /
   `$7E20E1` en el dominio `WRAM` (offsets `0x20E0` / `0x20E1`): los dos
   números tienen que coincidir frame a frame.

Además el overlay `gui.text` dibuja la misma línea sobre la imagen del juego,
lo que confirma que el bucle avanza un frame por frame real.

## Formato de salida

```
INIT   f=312     pad=0000  piece[type=0 rot=0 x=3 y=0]  lines=0
CHANGE f=347     pad=0200  piece[type=0 rot=0 x=2 y=0]  lines=0
IDLE   f=407     pad=0000  piece[type=0 rot=0 x=2 y=0]  lines=0
```

- `INIT` — primera lectura.
- `CHANGE` — algún valor observado cambió respecto al frame anterior.
- `IDLE` — latido cada 60 frames cuando nada cambia; demuestra que el bucle
  sigue vivo sin inundar la consola a 60 líneas/segundo.
