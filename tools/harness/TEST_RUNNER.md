# Test Runner interno

El Test Runner vive **dentro de la ROM** y es el único componente autorizado a
escribir `TestStatus`. Nada de gameplay (`board.c`, `piece.c`, `render.c`,
`input.c`) toca ni el runner ni la estructura.

Ver `tools/harness/TEST_STATUS.md` para el contrato de memoria y cómo localizarlo
desde `apotris.sym`.

Archivos: `snes/source/test_runner.h`, `snes/source/test_runner.c`.

## Máquina de estados

```
        IDLE ──── test_runner_start(id) ────► RUNNING
                                                 │
                                    test_runner_update()
                                                 │
                                        ┌────────┴────────┐
                                        ▼                 ▼
                                      PASS              FAIL
```

Los estados **son** los valores de `TestStatus.status` (`TEST_STATUS_IDLE`,
`RUNNING`, `PASS`, `FAIL`).

Decisión de diseño: el runner **no** guarda su propio estado interno.
`test_status.status` es la única fuente de verdad. Una copia interna sería un
segundo estado que se puede desincronizar de lo que lee el harness, y no aporta
nada.

Una vez en PASS o FAIL el runner se queda ahí: `test_runner_update()` es no-op y
el resultado permanece legible indefinidamente. Para volver a arrancar hay que
llamar a `test_runner_start()` otra vez.

## API

```c
void test_runner_init(void);
void test_runner_start(u8 test_id);
void test_runner_update(void);
u8   test_runner_is_running(void);
```

| Función | Qué hace |
|---|---|
| `test_runner_init()` | Publica el contrato (`magic`, `version`) y deja `status` en `IDLE`. Una vez al arrancar |
| `test_runner_start(id)` | `status` = `RUNNING`, `test_id` = `id`, limpia `flags`/`expected`/`actual` |
| `test_runner_update()` | Avanza la prueba en curso. No-op si `status` != `RUNNING` |
| `test_runner_is_running()` | `1` si `status` == `RUNNING`, `0` si no |

Dos ajustes respecto a la API propuesta en la Story:

- **`u8` en vez de `bool`** para el retorno de `test_runner_is_running()`. Evita
  depender de `<stdbool.h>` en este toolchain; el resto del proyecto ya usa
  `u8`/`u16` de `snes.h`.
- **`u8 test_id` en vez de `uint8_t`**, por la misma convención del proyecto.

`test_runner_start()` llamada mientras hay una prueba en curso **la reinicia**.
No hay cola ni scheduler: eso es deliberado.

## Enganche en la ROM

Tres líneas en `snes/source/main.c`, todas fuera de la lógica de juego:

```c
int main(void)
{
    test_runner_init();
    test_runner_start(TEST_ID_SELFCHECK);
    ...
    while (1)
    {
        test_runner_update();
        ... gameplay sin cambios ...
    }
}
```

- `init` + `start` son las primeras sentencias de `main()`, antes de cualquier
  inicialización de vídeo o de tablero.
- `update` es la primera sentencia del frame loop, antes del bloque de input.

`main.c` ya no incluye `test_status.h` ni llama a `test_status_init()`: ahora eso
pasa por el runner, así que el runner queda como único escritor de la estructura.

Gravedad, movimiento, colisiones y generación de piezas quedan intactos. El único
efecto del runner sobre el juego es el coste de una comparación por frame cuando
no hay prueba en curso.

## Prueba inicial: `TEST_ID_SELFCHECK` (id 1)

Trivial a propósito. No prueba nada del juego: valida el camino
ROM → WRAM → harness.

```c
test_status.expected = TEST_SELFCHECK_VALUE;   /* 0x1234 */
test_status.actual   = test_status.expected;   /* ida y vuelta por WRAM */
status = (actual == TEST_SELFCHECK_VALUE) ? PASS : FAIL;
```

`0x1234` está elegido para no colisionar con WRAM virgen (`0x5555`), ni con
`0x0000` ni `0xFFFF`: si el harness lee `0x1234` en `actual`, es porque la ROM
lo escribió.

### `test_id` desconocido

El `default` del `switch` termina en **FAIL** con el id en `actual`. Nunca se
queda en `RUNNING`: eso dejaría al harness esperando un resultado que no llega,
y un timeout no distingue "prueba colgada" de "prueba inexistente".

## Cómo se registran pruebas futuras

Dos pasos, sin tocar el harness ni Lua ni Python:

**1. Agregar el id en `test_runner.h`, al final:**

```c
#define TEST_ID_NONE       0
#define TEST_ID_SELFCHECK  1
#define TEST_ID_GRAVITY    2   /* <- nuevo */
```

**2. Agregar un `case` en el `switch` de `test_runner_update()`:**

```c
case TEST_ID_GRAVITY:
    test_status.expected = <valor esperado>;
    test_status.actual   = <valor observado>;
    test_status.status = (test_status.actual == test_status.expected)
                         ? TEST_STATUS_PASS : TEST_STATUS_FAIL;
    break;
```

Reglas:

- Los ids **nunca** se reciclan ni se reordenan: el harness los reporta tal
  cual, así que un id reutilizado hace que un log viejo signifique otra cosa.
- Los ids nuevos van **al final**.
- Una prueba que necesite varios frames simplemente **no** escribe `status` en
  su `case`: al seguir en `RUNNING`, `test_runner_update()` vuelve a entrar en el
  frame siguiente. El contador de frames que necesite va como `static` dentro de
  `test_runner.c`, no en `TestStatus`.
- Ninguna prueba debe modificar estado de juego. Si una prueba futura necesita
  provocar situaciones concretas del tablero, eso es una decisión de diseño
  aparte, no algo que se cuele en un `case`.

El harness no necesita cambios para pruebas nuevas: sigue leyendo los mismos
10 bytes y reporta `test_id`/`expected`/`actual` sin interpretarlos.

## Verificación

`make` compila y enlaza. Símbolos en `apotris.sym`:

```
0000da1a test_runner_init
0000d8cc test_runner_start
0000d2e3 test_runner_update
0000d7e3 test_status_init
007e20e7 test_status
```

`test_runner_is_running` **no** aparece: todavía no la llama nadie. Explicación
esperable: el linker descarta la sección no referenciada. No lo verifiqué
directamente (implicaría entrar en wlalink, fuera del proyecto). Es inofensivo —
compila, y aparecerá en el `.sym` cuando una Story la use.

### Valores esperados en WRAM tras el arranque

`TestStatus` está en bus `$7E20E7` → offset `0x20E7` del dominio `WRAM`.

| Offset | Campo | Valor esperado |
|---:|---|---|
| `0x20E7` | `magic` (u16 LE) | `0x5453` |
| `0x20E9` | `version` | `1` |
| `0x20EA` | `status` | `2` (PASS) |
| `0x20EB` | `test_id` | `1` (SELFCHECK) |
| `0x20EC` | `flags` | `0` |
| `0x20ED` | `expected` (u16 LE) | `0x1234` |
| `0x20EF` | `actual` (u16 LE) | `0x1234` |

Los 10 bytes crudos desde `0x20E7`:

```
53 54 01 02 01 00 34 12 34 12
```

Durante el arranque (entre `test_runner_start()` y el primer
`test_runner_update()` del frame loop) `status` vale `1` (RUNNING). Al entrar el
loop pasa a `2` y se queda ahí.

Comprobable a mano en BizHawk: **Tools → RAM Watch**, dominio `WRAM`, dirección
`0x20E7`, 2 bytes little-endian → `0x5453`; y `0x20EA` de 1 byte → `2`.

La verificación automática de estos valores requiere Lua/Python nuevos, que esta
Story excluye explícitamente.
