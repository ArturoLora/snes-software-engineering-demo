/*---------------------------------------------------------------------------------

    test_status.h - contrato publico de depuracion entre la ROM y el Test Harness.

    Esta estructura es la UNICA interfaz que el harness debe leer. Existe para
    desacoplar el harness de las variables internas del juego (gs.piece.x,
    gs.lines.count, ...): esas pueden cambiar de layout o desaparecer cuando
    evolucione la implementacion; TestStatus no.

    Reglas del contrato:

    - Pequena, plana y estable. Solo u8/u16, sin punteros, sin arrays anidados.
    - Solo se agregan campos AL FINAL, y cada vez que el layout cambia sube
      TEST_STATUS_VERSION. Nunca se reordenan ni se reciclan campos existentes.
    - Todos los campos son u8/u16, asi que el layout es exactamente el orden de
      declaracion (este toolchain no inserta padding; verificado contra
      GameState en apotris.sym).
    - Vive en WRAM. El harness localiza su direccion leyendo apotris.sym, no
      hardcodeandola. Ver tools/harness/TEST_STATUS.md.

    Esta Story solo PUBLICA la API. No hay test runner, ni modo de pruebas, ni
    logica que produzca PASS/FAIL: el juego se limita a inicializar la
    estructura al arrancar.

---------------------------------------------------------------------------------*/
#ifndef TEST_STATUS_H
#define TEST_STATUS_H

#include <snes.h>

/* Handshake. El harness debe comprobar magic ANTES de creer cualquier otro
   campo: bsnes rellena la WRAM sin inicializar con 0x55, asi que una lectura
   virgen da 0x5555. 0x5453 ('S','T' en bytes) es distinguible de ese patron y
   tambien de 0x0000 y 0xFFFF. */
#define TEST_STATUS_MAGIC 0x5453

/* Version del layout. Subir SIEMPRE que cambie el conjunto o el orden de
   campos, para que un harness viejo detecte un contrato que no entiende. */
#define TEST_STATUS_VERSION 1

/* Valores de `status`. Vocabulario del contrato; esta Story no los produce. */
#define TEST_STATUS_IDLE 0    /* estructura publicada, ninguna prueba en curso */
#define TEST_STATUS_RUNNING 1 /* prueba en curso */
#define TEST_STATUS_PASS 2    /* prueba terminada, resultado correcto */
#define TEST_STATUS_FAIL 3    /* prueba terminada, resultado incorrecto */

typedef struct
{
    u16 magic;    /* +0  TEST_STATUS_MAGIC */
    u8 version;   /* +2  TEST_STATUS_VERSION */
    u8 status;    /* +3  uno de TEST_STATUS_* */
    u8 test_id;   /* +4  que prueba reporta (0 = ninguna) */
    u8 flags;     /* +5  reservado, bits libres para el futuro */
    u16 expected; /* +6  valor esperado */
    u16 actual;   /* +8  valor observado */
} TestStatus;     /* 10 bytes */

/* Definicion en test_status.c. NO es static a proposito: un simbolo global da
   un nombre estable en apotris.sym. */
extern TestStatus test_status;

/* Publica el contrato: magic, version y estado inicial limpio. Llamar una vez
   al arrancar, antes de cualquier logica de juego. Sin esto la estructura
   queda con la WRAM virgen (0x55) y el harness no puede distinguirla de
   basura. */
void test_status_init(void);

#endif
