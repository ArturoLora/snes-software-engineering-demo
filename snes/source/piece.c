#include "piece.h"
#include "board.h"

/* Adapted from Apotris Game::next() (reference/apotris/source/tetrisEngine.cpp:1202-1274):
   initial position only (pawn.y = lengthY/2, pawn.x = lengthX/2-2). No queue/7-bag
   yet, so type is temporarily fixed to 0. No collision/top-out check, no lock,
   no render, no movement, no gravity — those belong to future stories. */
void piece_spawn(GameState *gs)
{
    gs->piece.type = 0;
    gs->piece.rotation = 0;
    gs->piece.x = BOARD_WIDTH / 2 - 2;
    gs->piece.y = 2;
}

/* Adapted from Apotris Game::moveLeft()/moveRight() (tetrisEngine.cpp:286-332):
   if (checkRotation(dx, 0, rotation)) pawn.x += dx; else no-op. We don't have
   full-shape collision yet (piece_shapes isn't wired to any collision check),
   so board_is_cell_occupied() validates piece.x/piece.y as a single point —
   an explicit simplification until full-shape collision exists. No DAS/ARR,
   no moveHistory/finesse, no lockCheck(), no sounds/gameMode. */
void piece_move_left(GameState *gs)
{
    s8 new_x = gs->piece.x - 1;
    if (!board_is_cell_occupied(gs, new_x, gs->piece.y))
        gs->piece.x = new_x;
}

void piece_move_right(GameState *gs)
{
    s8 new_x = gs->piece.x + 1;
    if (!board_is_cell_occupied(gs, new_x, gs->piece.y))
        gs->piece.x = new_x;
}
