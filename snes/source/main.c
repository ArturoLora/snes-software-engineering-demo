/*---------------------------------------------------------------------------------

    Story 1.1 - Boot de ROM e inicializacion minima de PVSnesLib.
    Story 1.2 - BG1: playfield vacio (tileset+mapa fijo 10x20, sin logica de tablero).
    Sin board/pieza/render real todavia (eso llega en stories futuras).

    Story 2.1 - Tablero logico estatico (board.c/game_state.h), sin piezas/gravedad/
    colision/render de tablero todavia.
    Story 2.2 - board_is_cell_occupied: bordes/piso + celda ocupada, sin pieza/
    spawn/rotacion todavia.
    Story 3.1 - ActivePiece agregado a GameState (solo el dato: type/rotation/x/y).
    Sin spawn, sin movimiento, sin render, sin rotacion real todavia.
    Story 3.2 - piece_data: tabla de formas (rotacion 0) de las 7 piezas.
    Sin spawn, sin render, sin movimiento, sin colision de forma, sin rotacion.
    Story 3.3 - piece_spawn(): pieza fija (type=0) en posicion inicial. Sin
    render, movimiento, gravedad, colisiones, lock, 7-bag ni top-out todavia.
    Story 3.4 - piece_move_left()/piece_move_right(): mueven piece.x validado
    contra board_is_cell_occupied() (colision puntual, no de forma completa).
    Sin pad real, DAS, ARR, gravedad, lock, render ni rotacion todavia.
    Story 3.5 - piece_apply_gravity(): un paso fijo de caida (piece.y++) por
    llamada, validado con board_is_cell_occupied() (misma simplificacion
    puntual). Sin acumulador Q8.8, velocidad por nivel, lock, top-out, line
    clear, render ni pad todavia.
    Story 3.6 - piece_lock(): escribe piece.x/piece.y en board[][] via
    board_set() (un unico punto, no la forma completa). Sin line clear,
    top-out, nueva pieza, lock delay, render, gravedad Q8.8 todavia.

    Nota de layout (post-3.5): las pruebas de debug de todas las stories
    excedian la altura visible de la consola (filas hasta 32). Se
    condensaron/reordenaron para que todas quepan a la vez (filas 2-16) -
    ningun cambio de logica/funcionalidad, solo donde y como se imprime.

---------------------------------------------------------------------------------*/
#include <snes.h>
#include "game_state.h"
#include "board.h"
#include "piece_data.h"
#include "piece.h"

extern char tilfont, palfont;
extern char playfieldtiles, playfieldtiles_end;
extern char playfieldpal, playfieldpal_end;
extern char playfieldmap, playfieldmap_end;

unsigned short pad0;
static GameState gs;

//---------------------------------------------------------------------------------
int main(void)
{
    // Initialize text console with our font
    consoleSetTextMapPtr(0x6800);
    consoleSetTextGfxPtr(0x3000);
    consoleSetTextOffset(0x0100);
    consoleInitText(0, 16 * 2, &tilfont, &palfont);

    // Init background 0 (debug text console)
    bgSetGfxPtr(0, 0x2000);
    bgSetMapPtr(0, 0x6800, SC_32x32);

    // Init background 1 (playfield, static tileset+map, palette entry 1 so it
    // doesn't share CGRAM slot 0 with the console font)
    bgInitTileSet(1, (u8 *)&playfieldtiles, (u8 *)&playfieldpal, 1,
                  (u16)(&playfieldtiles_end - &playfieldtiles),
                  (u16)(&playfieldpal_end - &playfieldpal),
                  BG_16COLORS, 0x4000);
    bgInitMapSet(1, (u8 *)&playfieldmap, (u16)(&playfieldmap_end - &playfieldmap),
                 SC_32x32, 0x7000);

    // Now put in 16 color mode and disable the unused Bg
    setMode(BG_MODE1, 0);
    bgSetDisable(2);

    // Draw a wonderful text :P
    consoleDrawText(6, 2, "Hello Apotris SNES");
    consoleDrawText(3, 4, "PRESS A PAD BUTTON");

    // Stories 2.1/2.2/3.2 - board/collision/piece_data setup, unchanged: the
    // test cell (3,5) set here is still relied on by the Story 3.5 gravity
    // test below (step 3 collides against it) - only the dedicated per-story
    // print lines were condensed into one row (B/C/D) to save vertical space.
    board_init(&gs);
    board_set(&gs, 3, 5, 7);
    // u8/s8 values are not promoted to a full 16-bit slot when passed
    // directly as varargs on this toolchain (consoleDrawText's %u/%d reader
    // always consumes 2 bytes per argument) - explicit widening casts are
    // required, or each argument after the first shifts by a byte.
    consoleDrawText(1, 6, "B:%u C:%u/%u/%u D:%u",
                     (u16)board_get(&gs, 3, 5),
                     (u16)board_is_cell_occupied(&gs, 3, 5),
                     (u16)board_is_cell_occupied(&gs, 0, 0),
                     (u16)board_is_cell_occupied(&gs, -1, 0),
                     (u16)piece_shapes[0][1][0]);

    // Story 3.3 - minimal spawn test: piece_spawn() sets gs.piece (fixed
    // type=0, initial position). No movement, no render, no gravity, no
    // collision, no lock.
    piece_spawn(&gs);
    consoleDrawText(1, 8, "PIECE: %u %u %d %d",
                     (u16)gs.piece.type, (u16)gs.piece.rotation,
                     (s16)gs.piece.x, (s16)gs.piece.y);

    // Story 3.4 - minimal horizontal movement test: piece_move_left() then
    // piece_move_right(), condensed to one row. Only piece.x changes; no pad
    // input, no DAS/ARR, no gravity, no lock, no render, no rotation.
    {
        s16 x_before_left, x_after_left, x_before_right, x_after_right;

        x_before_left = (s16)gs.piece.x;
        piece_move_left(&gs);
        x_after_left = (s16)gs.piece.x;

        x_before_right = (s16)gs.piece.x;
        piece_move_right(&gs);
        x_after_right = (s16)gs.piece.x;

        consoleDrawText(1, 10, "MOVE L:%d>%d R:%d>%d",
                         x_before_left, x_after_left,
                         x_before_right, x_after_right);
    }

    // Story 3.5 - minimal gravity test: piece_spawn() again to reset y (the
    // movement test above only touched x), then 3 fixed gravity steps,
    // condensed to one row as a Y chain. No pad, no lock, no top-out, no
    // line clear, no render.
    piece_spawn(&gs);
    {
        s16 y0, y1, y2, y3;

        y0 = (s16)gs.piece.y;
        piece_apply_gravity(&gs);
        y1 = (s16)gs.piece.y;
        piece_apply_gravity(&gs);
        y2 = (s16)gs.piece.y;
        piece_apply_gravity(&gs);
        y3 = (s16)gs.piece.y;

        consoleDrawText(1, 12, "GRAV:%d>%d>%d>%d", y0, y1, y2, y3);
    }

    // Story 3.6 - minimal lock test: keep applying gravity (bounded loop)
    // until piece.y stops changing, then piece_lock(), then verify with
    // board_get() that the cell got occupied. Reuses the piece state left by
    // the Story 3.5 test above (already resting against the (3,5) test cell)
    // - no re-spawn, no top-out, no line clear, no render, no lock delay.
    {
        s16 prev_y;
        u8 iterations;

        iterations = 0;
        do {
            prev_y = (s16)gs.piece.y;
            piece_apply_gravity(&gs);
            iterations++;
        } while ((s16)gs.piece.y != prev_y && iterations < BOARD_HEIGHT);

        piece_lock(&gs);
        consoleDrawText(1, 14, "LOCK X:%d Y:%d VAL:%u",
                         (s16)gs.piece.x, (s16)gs.piece.y,
                         (u16)board_get(&gs, (u8)gs.piece.x, (u8)gs.piece.y));
    }

    bgSetEnable(1);
    setScreenOn();

    while (1)
    {
        // Get current #0 pad
        pad0 = padsCurrent(0);

        // Update display with current pad
        switch (pad0)
        {
        case KEY_A:
            consoleDrawText(6, 16, "A PRESSED     ");
            break;
        case KEY_B:
            consoleDrawText(6, 16, "B PRESSED     ");
            break;
        case KEY_SELECT:
            consoleDrawText(6, 16, "SELECT PRESSED");
            break;
        case KEY_START:
            consoleDrawText(6, 16, "START PRESSED ");
            break;
        case KEY_RIGHT:
            consoleDrawText(6, 16, "RIGHT PRESSED ");
            break;
        case KEY_LEFT:
            consoleDrawText(6, 16, "LEFT PRESSED  ");
            break;
        case KEY_DOWN:
            consoleDrawText(6, 16, "DOWN PRESSED  ");
            break;
        case KEY_UP:
            consoleDrawText(6, 16, "UP PRESSED    ");
            break;
        case KEY_R:
            consoleDrawText(6, 16, "R PRESSED     ");
            break;
        case KEY_L:
            consoleDrawText(6, 16, "L PRESSED     ");
            break;
        case KEY_X:
            consoleDrawText(6, 16, "X PRESSED     ");
            break;
        case KEY_Y:
            consoleDrawText(6, 16, "Y PRESSED     ");
            break;
        default:
            consoleDrawText(6, 16, "              ");
            break;
        }

        WaitForVBlank();
    }

    return 0;
}
