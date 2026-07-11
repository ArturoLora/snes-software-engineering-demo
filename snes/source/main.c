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

---------------------------------------------------------------------------------*/
#include <snes.h>
#include "game_state.h"
#include "board.h"

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
    consoleDrawText(6, 10, "Hello Apotris SNES");
    consoleDrawText(3, 14, "PRESS A PAD BUTTON");

    // Story 2.1 - minimal board test: write a known value to a test cell and
    // read it back, to verify board_init/board_set/board_get without any
    // render/gameplay logic.
    board_init(&gs);
    board_set(&gs, 3, 5, 7);
    consoleDrawText(3, 12, "BOARD TEST: %u", board_get(&gs, 3, 5));

    // Story 2.2 - minimal collision test: occupied cell (same test cell as
    // above), an empty in-range cell, and an out-of-range cell.
    consoleDrawText(1, 16, "COL OCC/EMPTY/OOB: %u %u %u",
                     board_is_cell_occupied(&gs, 3, 5),
                     board_is_cell_occupied(&gs, 0, 0),
                     board_is_cell_occupied(&gs, -1, 0));

    // Story 3.1 - minimal ActivePiece test: write test values into gs.piece
    // and read them back, to confirm the struct exists and is accessible.
    // No spawn, no movement, no render, no rotation logic.
    gs.piece.type = 0;
    gs.piece.rotation = 0;
    gs.piece.x = 4;
    gs.piece.y = 0;
    consoleDrawText(1, 20, "PIECE TEST: %u %u %d %d",
                     gs.piece.type, gs.piece.rotation, gs.piece.x, gs.piece.y);

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
            consoleDrawText(6, 18, "A PRESSED     ");
            break;
        case KEY_B:
            consoleDrawText(6, 18, "B PRESSED     ");
            break;
        case KEY_SELECT:
            consoleDrawText(6, 18, "SELECT PRESSED");
            break;
        case KEY_START:
            consoleDrawText(6, 18, "START PRESSED ");
            break;
        case KEY_RIGHT:
            consoleDrawText(6, 18, "RIGHT PRESSED ");
            break;
        case KEY_LEFT:
            consoleDrawText(6, 18, "LEFT PRESSED  ");
            break;
        case KEY_DOWN:
            consoleDrawText(6, 18, "DOWN PRESSED  ");
            break;
        case KEY_UP:
            consoleDrawText(6, 18, "UP PRESSED    ");
            break;
        case KEY_R:
            consoleDrawText(6, 18, "R PRESSED     ");
            break;
        case KEY_L:
            consoleDrawText(6, 18, "L PRESSED     ");
            break;
        case KEY_X:
            consoleDrawText(6, 18, "X PRESSED     ");
            break;
        case KEY_Y:
            consoleDrawText(6, 18, "Y PRESSED     ");
            break;
        default:
            consoleDrawText(6, 18, "              ");
            break;
        }

        WaitForVBlank();
    }

    return 0;
}
