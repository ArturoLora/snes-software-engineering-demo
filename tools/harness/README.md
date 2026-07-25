# Test Harness — primer orquestador

`harness.py` es el primer orquestador del Test Harness de Apotris SNES. Lanza
BizHawk desde Python, carga la ROM y el script Lua del PoC automáticamente,
espera evidencia de que se está leyendo memoria SNES frame a frame, y reporta
`PASS` o `FAIL`.

No toca código C, no crea `TestStatus`, no usa OCR, ni captura de pantalla, ni
automatización de teclado/ratón. Todo el control va por argumentos oficiales de
línea de comandos de BizHawk.

## Ejecución

```bash
DISPLAY=:0 python3 tools/harness/harness.py
```

Solo stdlib de Python 3. Sin dependencias.

Opciones:

| Flag | Para qué |
|---|---|
| `--timeout SEGS` | Espera máxima antes de declarar FAIL (def. 45) |
| `--rom RUTA` | Otra ROM (def. `snes/apotris.sfc`) |
| `--lua RUTA` | Otro script Lua (def. `tools/lua/poc_read_memory.lua`) |
| `--lua-log RUTA` | Archivo de log que escribe el script Lua |
| `--keep-open` | No cerrar BizHawk al terminar, para inspección manual |
| `--no-lua-console` | No abrir la ventana Lua Console |

Código de salida: `0` = PASS, `1` = FAIL.

## Qué hace exactamente

1. **Preflight** — verifica lanzador de BizHawk, ROM, script Lua, `mono` en
   PATH y sesión gráfica. Si algo falta, FAIL inmediato con el motivo.
2. **Limpia el entorno** (ver limitación 1).
3. **Borra el log anterior** del script Lua, para no dar por buena evidencia
   vieja.
4. **Lanza BizHawk** con:
   ```
   EmuHawkMono.sh --lua=<script.lua> --luaconsole <rom.sfc>
   ```
5. **Sondea el log** cada 0.25 s hasta cumplir criterios o agotar el timeout.
6. **Cierra BizHawk** (SIGTERM, y SIGKILL si se resiste).
7. **Reporta** PASS/FAIL. En FAIL imprime motivos más la cola de stdout/stderr
   de BizHawk desde `tools/harness/artifacts/`.

### Criterios de PASS

Los cuatro tienen que cumplirse:

1. El log declara el dominio de memoria elegido.
2. Hay ≥ 2 lecturas registradas.
3. La emulación avanzó ≥ 180 frames (~3 s) entre la primera y la última
   lectura → el bucle Lua corre frame a frame de verdad, no solo arrancó.
4. Al menos una lectura de `gs` sale del patrón `0x55` con el que bsnes rellena
   la WRAM virgen → prueba de que se lee estado de juego real y no RAM sin
   inicializar.

## Argumentos de BizHawk: qué está soportado

Verificado leyendo las cadenas **UTF-16** de
`tools/BizHawk-2.11.1-linux-x64/dll/BizHawk.Client.Common.dll`, donde vive el
ArgParser de esta build:

```bash
strings -el dll/BizHawk.Client.Common.dll | grep -oiE '^--[a-z0-9_-]+' | sort -u
```

Flags relevantes que existen: `--lua`, `--luaconsole`, `--load-state`,
`--load-slot`, `--movie`, `--config`, `--chromeless`, `--fullscreen`,
`--socket-ip`, `--socket-port`, `--socket-udp`, `--mmf`, `--url-get`,
`--url-post`, `--userdata`, `--version`. La ROM va como argumento posicional.

> Nota: un `strings` normal **no** los encuentra. Los assemblies .NET guardan
> las cadenas en UTF-16, así que hace falta `strings -el`. Un grep ASCII da
> falso negativo y lleva a concluir, erróneamente, que `--lua` no existe.

## Limitaciones encontradas

### 1. Terminal empaquetada en snap rompe el lanzamiento de Mono

Al lanzar desde la terminal integrada de VS Code (instalado como snap), BizHawk
muere antes de arrancar:

```
mono: symbol lookup error: /snap/core20/current/lib/x86_64-linux-gnu/libpthread.so.0:
      undefined symbol: __libc_pthread_init, version GLIBC_PRIVATE
```

Causa: el snap inyecta `GTK_PATH`, `GTK_EXE_PREFIX`, `LOCPATH`,
`GSETTINGS_SCHEMA_DIR`, etc. apuntando a `/snap/code/<rev>/...`, y GTK carga
librerías con una glibc distinta a la del sistema.

Mitigación implementada: `clean_env()` restaura cada variable desde su
`<VAR>_VSCODE_SNAP_ORIG` (el snap guarda ahí el valor original) y elimina las
que sigan apuntando a `/snap`. No es un problema de BizHawk ni del harness.

### 2. No hay modo headless

BizHawk es WinForms: necesita servidor gráfico. Esta build no tiene flag
headless (`--chromeless` solo quita adornos de ventana, no el display), y el
sistema no tiene `Xvfb` ni `xvfb-run` instalados. El harness exige `DISPLAY` o
`WAYLAND_DISPLAY` y falla en preflight si no hay.

Consecuencia para CI: hoy no se puede correr sin sesión gráfica. Instalar
`xvfb` lo desbloquearía, pero está fuera del alcance de esta Story.

### 3. No hay forma por CLI de terminar tras N frames

No existe flag tipo `--frames=N` ni `--exit-after`. Sí existen
`client.exit()` y `client.exitCode(n)`, pero **solo son invocables desde Lua**,
y el script del PoC hace bucle infinito y nunca los llama.

Consecuencia: el harness es quien decide cuándo parar (criterios de PASS o
timeout) y cierra el proceso con SIGTERM. Cuando exista un script Lua propio del
harness, la vía limpia es que ese script llame a `client.exitCode(0|1)` y
Python lea el código de salida.

### 4. No hay canal Python←BizHawk que no requiera cooperación del Lua

Los canales IPC oficiales (`comm.*`) existen y están habilitados por CLI:

- `--socket-ip` / `--socket-port` / `--socket-udp` → `comm.socketServerSend(...)`
- `--mmf=<archivo>` → `comm.mmfWrite(...)`
- `--url-get` / `--url-post` → `comm.httpPost(...)`

Todos exigen que **el script Lua llame** a la función correspondiente. Ninguno
permite a Python sondear el estado del emulador por su cuenta.

Decisión para esta Story: usar el archivo de log que el PoC ya escribe con
`io.open`. Se verificó que la librería `io` de Lua **sí** está disponible en
esta build (el log se genera). Cuando el harness necesite un canal
bidireccional, `--socket-ip/--socket-port` es la opción oficial.

### 5. La documentación LuaCATS de la propia build está desactualizada

`Lua/_docs_luacats/memory.d.lua` dice que `memory.getmemorydomainlist()`
devuelve *"un string delimitado por saltos de línea"*. En 2.11.1 devuelve una
**tabla**. El PoC fallaba con:

```
NLua.Exceptions.LuaScriptException: [string "main"]:64: bad argument #1 to 'gmatch'
      (string expected, got table)
```

Corregido en `tools/lua/poc_read_memory.lua`: acepta tabla o string.

### 6. Este core SNES no expone dominio "System Bus"

Dominios reales según el propio emulador:

```
VRAM (65536 bytes)
CARTROM (262144 bytes)
Waterbox PageData (6869 bytes)
WRAM (131072 bytes)
```

No hay `System Bus`. El fallback que el PoC traía previsto para ese dominio
nunca se usa aquí; `WRAM` es el camino correcto, con offset 0 = bus `$7E0000`.

## Evidencia de funcionamiento

Corrida real (`--timeout 60`):

```
[harness] BizHawk lanzado (pid 216719); esperando hasta 60s
[harness] el script Lua esta escribiendo .../tools/lua/poc_read_memory.log
[harness] cerrando BizHawk

--------------------------------------------------------------
dominio de memoria .. WRAM
lecturas en el log .. 14
frames observados ... 0 -> 203 (+203)
estado de juego ..... si
--------------------------------------------------------------
PASS
```

Log producido por el script Lua en esa misma corrida:

```
Dominios de memoria del core actual:
  - VRAM (65536 bytes)
  - CARTROM (262144 bytes)
  - Waterbox PageData (6869 bytes)
  - WRAM (131072 bytes)

Sistema ....... SNES
Dominio usado . WRAM
Tamano dominio  131072 bytes
pad0 en bus $7E2000 -> offset 0x2000 del dominio
gs   en bus $7E2002 -> offset 0x2002 del dominio
--------------------------------------------------------------
INIT   f=0       pad=5555  piece[type=85 rot=85 x=85 y=85]  lines=85
CHANGE f=1       pad=0000  piece[type=85 rot=85 x=85 y=85]  lines=85
CHANGE f=9       pad=0000  piece[type=0 rot=0 x=3 y=2]  lines=85
CHANGE f=12      pad=0000  piece[type=0 rot=0 x=3 y=3]  lines=85
CHANGE f=15      pad=0000  piece[type=0 rot=0 x=3 y=3]  lines=0
CHANGE f=16      pad=0000  piece[type=0 rot=0 x=3 y=3]  lines=1
CHANGE f=21      pad=0000  piece[type=0 rot=0 x=3 y=3]  lines=0
CHANGE f=23      pad=0000  piece[type=0 rot=0 x=3 y=2]  lines=0
CHANGE f=53      pad=0000  piece[type=0 rot=0 x=3 y=3]  lines=0
CHANGE f=83      pad=0000  piece[type=0 rot=0 x=3 y=4]  lines=0
CHANGE f=113     pad=0000  piece[type=0 rot=0 x=3 y=5]  lines=0
CHANGE f=143     pad=0000  piece[type=0 rot=0 x=3 y=6]  lines=0
CHANGE f=173     pad=0000  piece[type=0 rot=0 x=3 y=7]  lines=0
CHANGE f=203     pad=0000  piece[type=0 rot=0 x=3 y=8]  lines=0
```

Lo que esto demuestra, punto por punto:

- **f=0/f=1 con `0x55`** — arranque antes de que el juego inicialice: es
  exactamente el patrón de WRAM virgen de bsnes. Confirma que se lee RAM real
  y no una constante inventada.
- **f=9 en adelante** — `gs` pasa a valores plausibles (`type=0 rot=0 x=3`):
  el juego ya escribió, y las direcciones derivadas del `.sym` apuntan al sitio
  correcto.
- **`y` sube 1 cada exactamente 30 frames** (f=83 → 113 → 143 → 173 → 203):
  coincide con la gravedad automática de la Story 4.3. Corroboración
  independiente de que `$7E20E1` es `gs.piece.y`.
- **`lines` cambia 85 → 0 → 1 → 0** — se observa el ciclo de detección de
  líneas en `gs.lines.count`.

## Artefactos generados

| Ruta | Qué es |
|---|---|
| `tools/harness/artifacts/bizhawk_stdout.txt` | stdout de BizHawk de la última corrida |
| `tools/harness/artifacts/bizhawk_stderr.txt` | stderr de BizHawk de la última corrida |
| `tools/lua/poc_read_memory.log` | log que escribe el script Lua |

Los tres se sobrescriben en cada corrida y no aportan nada al repositorio;
conviene ignorarlos en Git.
