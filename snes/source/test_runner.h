/*---------------------------------------------------------------------------------

    test_runner.h - Test Runner interno, desacoplado del gameplay.

    Unico componente autorizado a escribir TestStatus. Nada de gameplay
    (board.c, piece.c, render.c, input.c) toca esta estructura ni este modulo.

    Maquina de estados, la minima que cumple el contrato:

        IDLE ---- test_runner_start(id) ----> RUNNING
                                                |
                                 test_runner_update()
                                                |
                                        +-------+-------+
                                        v               v
                                      PASS            FAIL

    El estado NO se guarda por duplicado: `test_status.status` ES el estado de
    la maquina. Un segundo campo interno seria una copia que se puede
    desincronizar de lo que lee el harness.

    Sin framework: sin macros de assert, sin suites, sin scheduler, sin
    callbacks, sin punteros a funcion. El despacho por test_id es un `switch`.

---------------------------------------------------------------------------------*/
#ifndef TEST_RUNNER_H
#define TEST_RUNNER_H

#include <snes.h>

/* Registro de pruebas. Un id por prueba, valores estables: el harness los
   reporta tal cual, asi que nunca se reciclan ni se reordenan. Las Stories
   futuras agregan ids AL FINAL. */
#define TEST_ID_NONE 0      /* ninguna prueba */
#define TEST_ID_SELFCHECK 1 /* validacion de la propia infraestructura */

/* Valor que TEST_ID_SELFCHECK escribe y vuelve a leer por WRAM. Elegido para
   no coincidir con WRAM virgen (0x5555), ni con 0x0000 / 0xFFFF. */
#define TEST_SELFCHECK_VALUE 0x1234

/* Publica TestStatus y deja el runner en IDLE. Llamar una vez al arrancar,
   antes de cualquier logica de juego. */
void test_runner_init(void);

/* Pasa a RUNNING con la prueba indicada y limpia expected/actual/flags.
   Llamarla mientras hay una prueba en curso la reinicia; no se encola nada
   (no hay scheduler). */
void test_runner_start(u8 test_id);

/* Avanza la prueba en curso. No hace nada si el runner no esta en RUNNING, asi
   que es seguro llamarla cada frame de forma incondicional. Cuando la prueba
   termina, escribe expected/actual y deja status en PASS o FAIL. */
void test_runner_update(void);

/* 1 si hay una prueba en curso, 0 si no. Devuelve u8 y no `bool` para no
   depender de <stdbool.h> en este toolchain. */
u8 test_runner_is_running(void);

#endif
