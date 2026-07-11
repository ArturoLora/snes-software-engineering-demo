#include "board.h"

/* Sole owner of gs->board.board — no other module writes this array directly. */

void board_clear(GameState *gs)
{
    u8 x, y;
    for (y = 0; y < BOARD_HEIGHT; y++)
        for (x = 0; x < BOARD_WIDTH; x++)
            gs->board.board[y][x] = 0;
}

void board_init(GameState *gs)
{
    board_clear(gs);
}

u8 board_get(GameState *gs, u8 x, u8 y)
{
    if (x >= BOARD_WIDTH || y >= BOARD_HEIGHT)
        return 0;
    return gs->board.board[y][x];
}

void board_set(GameState *gs, u8 x, u8 y, u8 value)
{
    if (x >= BOARD_WIDTH || y >= BOARD_HEIGHT)
        return;
    gs->board.board[y][x] = value;
}

/* Adapted from Apotris Game::checkRotation's per-cell check
   (reference/apotris/source/tetrisEngine.cpp:34) — border/floor bounds and
   occupied-cell test, without the 4x4 pawn shape loop (no ActivePiece yet). */
u8 board_is_cell_occupied(GameState *gs, s8 x, s8 y)
{
    if (x < 0 || x >= BOARD_WIDTH || y < 0 || y >= BOARD_HEIGHT)
        return 1;
    return board_get(gs, (u8)x, (u8)y) != 0;
}
