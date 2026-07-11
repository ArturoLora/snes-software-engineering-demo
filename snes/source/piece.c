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

/* Adapted from the gravity block inside Apotris Game::update()
   (tetrisEngine.cpp:543-559): if (checkRotation(0,1,rotation)) pawn.y++;
   No speedCounter/speed accumulator, no per-level gravity table, no
   NRS-branch place() — one call here always applies exactly one step,
   same point-collision simplification as piece_move_left/right. */
void piece_apply_gravity(GameState *gs)
{
    s8 new_y = gs->piece.y + 1;
    if (!board_is_cell_occupied(gs, gs->piece.x, new_y))
        gs->piece.y = new_y;
}

/* Adapted from the board-write section of Apotris Game::place()
   (tetrisEngine.cpp:655-696): board[y][x] = pawn.current + shape_bits, looped
   over the 4x4 shape. No shape loop here — same point simplification as
   piece_move_left/right/piece_apply_gravity: only piece.x/piece.y is written,
   via board_set(). Value is type+1 (never 0, matches the 0=empty convention
   from board_get/board_set). No top-out check, no lock delay, no re-spawn —
   the piece stays at its last position after locking. */
void piece_lock(GameState *gs)
{
    board_set(gs, (u8)gs->piece.x, (u8)gs->piece.y, gs->piece.type + 1);
}
