/*---------------------------------------------------------------------------------

    test_runner.c - implementacion del Test Runner interno.

    Ver test_runner.h para el contrato y tools/harness/TEST_RUNNER.md para el
    flujo completo y como registrar pruebas nuevas.

---------------------------------------------------------------------------------*/
#include "test_runner.h"
#include "test_status.h"

void test_runner_init(void)
{
    /* test_status_init() deja magic/version publicados y status en IDLE. */
    test_status_init();
}

void test_runner_start(u8 test_id)
{
    test_status.test_id = test_id;
    test_status.flags = 0;
    test_status.expected = 0;
    test_status.actual = 0;
    test_status.status = TEST_STATUS_RUNNING;
}

void test_runner_update(void)
{
    if (test_status.status != TEST_STATUS_RUNNING)
        return;

    switch (test_status.test_id)
    {
    case TEST_ID_SELFCHECK:
        /* Prueba trivial de infraestructura: escribe un valor conocido en
           TestStatus y lo vuelve a leer. No comprueba nada del juego; solo
           que el camino ROM -> WRAM -> harness funciona de punta a punta. */
        test_status.expected = TEST_SELFCHECK_VALUE;
        test_status.actual = test_status.expected;

        if (test_status.actual == TEST_SELFCHECK_VALUE)
            test_status.status = TEST_STATUS_PASS;
        else
            test_status.status = TEST_STATUS_FAIL;
        break;

    default:
        /* test_id que este build no conoce. FAIL explicito con el id en
           `actual`: quedarse en RUNNING dejaria al harness esperando un
           resultado que nunca llega. */
        test_status.expected = 0;
        test_status.actual = (u16)test_status.test_id;
        test_status.status = TEST_STATUS_FAIL;
        break;
    }
}

u8 test_runner_is_running(void)
{
    return (test_status.status == TEST_STATUS_RUNNING) ? 1 : 0;
}
