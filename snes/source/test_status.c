/*---------------------------------------------------------------------------------

    test_status.c - definicion del contrato publico de depuracion.

    Vive en su propia unidad de traduccion a proposito: el simbolo que wlalink
    emite en apotris.sym queda ligado a este archivo y no se mueve cuando se
    reorganiza main.c.

    La estructura NO se declara con inicializador. En este toolchain los
    globales con inicializador requieren copia ROM->RAM al arrancar, asi que el
    valor inicial se escribe explicitamente en test_status_init().

---------------------------------------------------------------------------------*/
#include "test_status.h"

TestStatus test_status;

void test_status_init(void)
{
    test_status.magic = TEST_STATUS_MAGIC;
    test_status.version = TEST_STATUS_VERSION;
    test_status.status = TEST_STATUS_IDLE;
    test_status.test_id = 0;
    test_status.flags = 0;
    test_status.expected = 0;
    test_status.actual = 0;
}
