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

/* Adapted from the row-detection loop in Apotris Game::clear()
   (reference/apotris/source/tetrisEngine.cpp:870-878): for each row, a row
   is full if every column is non-zero. No T-spin/DIG/zone handling (scoring
   and other game modes, out of scope). Up to LINES_TO_CLEAR_MAX rows are
   recorded, matching game-architecture.md's LinesToClear (max 4 at once). */
u8 board_detect_full_lines(GameState *gs)
{
    u8 x, y, full;

    gs->lines.count = 0;
    for (y = 0; y < BOARD_HEIGHT; y++)
    {
        full = 1;
        for (x = 0; x < BOARD_WIDTH; x++)
            if (board_get(gs, x, y) == 0)
                full = 0;

        if (full && gs->lines.count < LINES_TO_CLEAR_MAX)
            gs->lines.rows[gs->lines.count++] = y;
    }

    return gs->lines.count;
}

/* Adapted from the shift-down loop in Apotris Game::removeClearLock()
   (reference/apotris/source/tetrisEngine.cpp:1624-1639): for each cleared
   row, every row above it shifts down by one (row 0 itself is never
   overwritten — assumed always empty, part of the top buffer). Collapses
   immediately (no clearLock/animation delay, per epics.md Story 2.3's
   explicit requirement). No COMBO-mode garbage refill, no next() spawn call. */
void board_collapse_lines(GameState *gs)
{
    u8 i, j, x, row;

    for (i = 0; i < gs->lines.count; i++)
    {
        row = gs->lines.rows[i];
        for (j = row; j > 0; j--)
            for (x = 0; x < BOARD_WIDTH; x++)
                board_set(gs, x, j, board_get(gs, x, j - 1));
    }

    gs->lines.count = 0;
}
