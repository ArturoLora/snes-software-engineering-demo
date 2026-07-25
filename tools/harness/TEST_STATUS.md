# TestStatus — contrato ROM ↔ Test Harness

`TestStatus` es la **única** estructura que el Test Harness debe leer de la
memoria de la SNES. Existe para desacoplar el harness de las variables internas
del juego.

Hasta ahora el harness leía `gs.piece.x`, `gs.piece.y`, `gs.lines.count`, etc.
Eso lo ataba al layout de `GameState`: cualquier campo nuevo, reordenado o
eliminado en `snes/source/game_state.h` rompía el harness en silencio (los
offsets se corren y se leen bytes de otro campo, sin error). `TestStatus` es una
superficie estable y versionada que el juego publica a propósito.

## Definición

`snes/source/test_status.h`:

```c
#define TEST_STATUS_MAGIC   0x5453
#define TEST_STATUS_VERSION 1

#define TEST_STATUS_IDLE    0
#define TEST_STATUS_RUNNING 1
#define TEST_STATUS_PASS    2
#define TEST_STATUS_FAIL    3

typedef struct
{
    u16 magic;    /* +0 */
    u8  version;  /* +2 */
    u8  status;   /* +3 */
    u8  test_id;  /* +4 */
    u8  flags;    /* +5 */
    u16 expected; /* +6 */
    u16 actual;   /* +8 */
} TestStatus;     /* 10 bytes */
```

| Offset | Tamaño | Campo | Significado |
|---:|---:|---|---|
| +0 | 2 | `magic` | `0x5453`. Handshake: si no coincide, no leer nada más |
| +2 | 1 | `version` | Versión del layout (hoy `1`) |
| +3 | 1 | `status` | `0` IDLE · `1` RUNNING · `2` PASS · `3` FAIL |
| +4 | 1 | `test_id` | Qué prueba reporta (`0` = ninguna) |
| +5 | 1 | `flags` | Reservado |
| +6 | 2 | `expected` | Valor esperado |
| +8 | 2 | `actual` | Valor observado |

Todos los campos son `u8`/`u16`, así que el layout es exactamente el orden de
declaración: este toolchain no inserta padding (ya verificado con `GameState`,
cuyo tamaño calculado coincidió byte a byte con el símbolo siguiente en el
`.sym`).

Los `u16` son **little-endian** (65816).

### Por qué hay un `magic`

bsnes rellena la WRAM sin inicializar con `0x55`. Una lectura de RAM virgen da
`0x5555`, que es un valor perfectamente plausible. Sin handshake, el harness no
puede distinguir "la ROM publicó el contrato" de "estoy leyendo basura".
`0x5453` es distinguible de `0x5555`, de `0x0000` y de `0xFFFF`.

Esto ya se observó en la práctica: en el log del PoC los primeros frames muestran
`piece[type=85 rot=85 x=85 y=85]` — `85` = `0x55`, WRAM virgen.

### Reglas de versionado

- Los campos existentes **nunca** se reordenan, se renombran ni se reciclan.
- Los campos nuevos se agregan **al final**.
- Cualquier cambio de layout **sube** `TEST_STATUS_VERSION`.
- El harness debe rechazar una `version` que no conozca, en vez de leer offsets
  a ciegas.

## Cómo localizarla sin hardcodear direcciones

La dirección **no es estable entre builds**. Es un global de WRAM y el linker lo
coloca según lo que haya antes; agregar o quitar cualquier otro global lo mueve.
Prueba concreta: `test_status` quedó en `007e20e7`, exactamente donde antes
estaba `tccs_libc_c.asm_msys` — ese símbolo se corrió hacia adelante en el mismo
build.

Por eso la dirección se obtiene de `snes/apotris.sym`, que `make` regenera en
cada compilación (`snes_rules` borra y reescribe el `.sym` en cada enlace).

### Formato del `.sym`

Dos líneas de comentario (`;`) y luego una línea por símbolo:

```
0000d730 test_status_init
007e20e7 test_status
```

El campo hexadecimal son 8 dígitos: **4 de banco + 4 de dirección**.

```
007e20e7
├──┘└──┘
banco  dirección   ->  banco 0x7E, dirección 0x20E7
```

La dirección de bus completa es `banco << 16 | direccion` = `$7E20E7`.

> Nota: `wlalink` emite `007e:20e7`; el Makefile de PVSnesLib le quita los dos
> puntos con `sed -i 's/://'`. También borra las líneas `SECTIONSTART_`,
> `SECTIONEND_` y `RAM_USAGE_SLOT_`, así que **no** hay que depender de esas: el
> símbolo del global de C sí sobrevive.

### Traducción a offset del dominio `WRAM` de BizHawk

El dominio `WRAM` son los 128 KiB de RAM de trabajo, con offset 0 = bus
`$7E0000`:

```
offset_wram = (banco << 16 | direccion) - 0x7E0000
```

Para `test_status`: `0x7E20E7 - 0x7E0000` = **`0x20E7`**.

Equivalente rápido: banco `0x7E` → offset = dirección; banco `0x7F` → offset =
`0x10000 + dirección`.

Si el símbolo cae en un banco distinto de `0x7E` / `0x7F`, no está en WRAM y hay
que tratarlo como error, no leerlo.

### Extracción (lo que el harness tendrá que hacer)

Buscar la línea cuyo nombre sea exactamente `test_status`:

```bash
grep -E '^[0-9a-f]{8} test_status$' snes/apotris.sym
# 007e20e7 test_status
```

Con el nombre exacto y anclado, no con `grep test_status` a secas: eso también
engancha `test_status_init`, que es una **función en ROM** (`0000d730`, banco 0)
y no una dirección de WRAM.

Pasos:

1. Leer `snes/apotris.sym`, saltar las líneas que empiezan con `;`.
2. Partir cada línea en `(hex, nombre)`.
3. Quedarse con `nombre == "test_status"`. Si no aparece → error: la ROM no
   publica el contrato (o el `.sym` es viejo; correr `make`).
4. `banco = int(hex[:4], 16)`, `direccion = int(hex[4:], 16)`.
5. Verificar `banco in (0x7E, 0x7F)`; si no, error.
6. `offset_wram = (banco << 16 | direccion) - 0x7E0000`.
7. Leer 10 bytes desde `offset_wram` en el dominio `WRAM`.
8. Validar `magic == 0x5453`. Si no coincide, no interpretar el resto.
9. Validar `version`. Si es desconocida, abortar.

## Estado en `apotris.sym` (evidencia)

Tras `make`:

```
$ grep -inE 'test_status' snes/apotris.sym
208:0000d730 test_status_init
209:007e20e7 test_status
```

- `test_status` → banco `0x7E`, dirección `0x20E7` → bus `$7E20E7` → offset
  `0x20E7` del dominio `WRAM`.
- `test_status_init` → banco `0x00` (ROM), es la función, no el dato.

El símbolo aparece sin prefijo `tccs_*` porque `test_status` es un global no
`static`. Los `static` salen como `tccs_<archivo>.asm_<nombre>` — comparar con
`007e2002 tccs_source/main.asm_gs`, que es `static GameState gs`.

### Determinismo

Recompilar sin cambiar fuentes da el mismo resultado exacto:

```
sym antes:   007e20e7 test_status
sym despues: 007e20e7 test_status
md5 antes:   95af9ca16797facdf6c3ac4653f2bfba
md5 despues: 95af9ca16797facdf6c3ac4653f2bfba
```

O sea: la dirección es **estable dentro de un mismo estado de fuentes**, y solo
se mueve si cambia la asignación de globales. Ese es justamente el motivo de
leerla del `.sym` en cada corrida en lugar de fijarla en el código.

### Integración al build

`source/test_status.c` se integró al build sin tocar el `Makefile`:
`snes_rules` hace `CFILES = $(wildcard $(SRC)/*.c)` y regenera `linkfile` en
cada compilación, así que `source/test_status.obj` apareció solo.

## Quién escribe esta estructura

El **Test Runner interno** (`snes/source/test_runner.c`) y nadie más. Ver
`tools/harness/TEST_RUNNER.md`.

`main.c` no toca `TestStatus` directamente: llama a `test_runner_init()` y
`test_runner_start()` al arrancar, y a `test_runner_update()` una vez por frame.
Nada de gameplay (`board.c`, `piece.c`, `render.c`, `input.c`) la referencia.

No se usa un inicializador en la declaración (`TestStatus test_status = {...}`)
porque en este toolchain los globales con inicializador exigen copia ROM→RAM al
arrancar; el valor inicial se escribe en `test_status_init()`.

## Estado actual del harness

El harness **todavía no lee `TestStatus`**: sigue leyendo `gs` vía el PoC Lua.
Migrarlo es trabajo de una Story posterior — ese es justamente el acoplamiento
que esta estructura existe para eliminar.

La verificación en ejecución (leer `$7E20E7` y ver `0x5453`) requiere Lua nuevo.
Se puede comprobar a mano con **Tools → RAM Watch** de BizHawk: dominio `WRAM`,
dirección `0x20E7`, tipo 2 bytes little-endian → debe mostrar `0x5453`.
