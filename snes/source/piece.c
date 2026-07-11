#include "piece.h"

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
