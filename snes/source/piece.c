#include "piece.h"
#include "board.h"
#include "piece_data.h"

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

/* Adapted from Apotris Game::checkRotation's full 4x4 shape loop
   (reference/apotris/source/tetrisEngine.cpp:34): for every occupied cell of
   piece_shapes[type] at a hypothetical (x,y), test board_is_cell_occupied()
   per cell. Replaces the single-point simplification used by Stories
   3.4/3.5/3.6 — board.c's Story 2.2 comment explicitly deferred this loop
   (board.c doesn't know about ActivePiece/piece_data by design,
   game-architecture.md#2 — the loop belongs here instead). */
static u8 piece_shape_collides(GameState *gs, u8 type, s8 x, s8 y)
{
    u8 row, col;

    for (row = 0; row < PIECE_GRID_SIZE; row++)
        for (col = 0; col < PIECE_GRID_SIZE; col++)
            if (piece_shapes[type][row][col] &&
                board_is_cell_occupied(gs, (s8)(x + col), (s8)(y + row)))
                return 1;

    return 0;
}

/* Adapted from Apotris Game::moveLeft()/moveRight() (tetrisEngine.cpp:286-332):
   if (checkRotation(dx, 0, rotation)) pawn.x += dx; else no-op. Now validated
   against the full 4-cell shape via piece_shape_collides() instead of the
   single-point simplification used by Stories 3.4/3.5/3.6. No DAS/ARR, no
   moveHistory/finesse, no lockCheck(), no sounds/gameMode. */
void piece_move_left(GameState *gs)
{
    s8 new_x = gs->piece.x - 1;
    if (!piece_shape_collides(gs, gs->piece.type, new_x, gs->piece.y))
        gs->piece.x = new_x;
}

void piece_move_right(GameState *gs)
{
    s8 new_x = gs->piece.x + 1;
    if (!piece_shape_collides(gs, gs->piece.type, new_x, gs->piece.y))
        gs->piece.x = new_x;
}

/* Adapted from the gravity block inside Apotris Game::update()
   (tetrisEngine.cpp:543-559): if (checkRotation(0,1,rotation)) pawn.y++;
   No speedCounter/speed accumulator, no per-level gravity table, no
   NRS-branch place() — one call here always applies exactly one step, now
   validated against the full 4-cell shape via piece_shape_collides(). */
void piece_apply_gravity(GameState *gs)
{
    s8 new_y = gs->piece.y + 1;
    if (!piece_shape_collides(gs, gs->piece.type, gs->piece.x, new_y))
        gs->piece.y = new_y;
}

/* Story 4.8: the piece is resting when it cannot fall one more step. Adapted
   from the same checkRotation(0,1,rotation) test Apotris Game::update() uses
   to decide whether the lock timer should run (tetrisEngine.cpp:543-559) -
   here it decides the lock outright, because there is no lock delay yet.
   Lives in piece.c and not in main.c: this is a game rule, and main.c holds
   none (game-architecture.md#2). Reuses the existing shape collision. */
u8 piece_is_landed(GameState *gs)
{
    return piece_shape_collides(gs, gs->piece.type, gs->piece.x,
                                (s8)(gs->piece.y + 1));
}

/* Adapted from the board-write section of Apotris Game::place()
   (tetrisEngine.cpp:655-696): board[y][x] = pawn.current + shape_bits, looped
   over the 4x4 shape. Now writes all occupied cells of piece_shapes[type]
   (replaces the single-point simplification used by Stories 3.4/3.5/3.6) via
   board_set(). Value is type+1 (never 0, matches the 0=empty convention from
   board_get/board_set). No top-out check, no lock delay, no re-spawn — the
   piece stays at its last position after locking. */
void piece_lock(GameState *gs)
{
    u8 row, col;

    for (row = 0; row < PIECE_GRID_SIZE; row++)
        for (col = 0; col < PIECE_GRID_SIZE; col++)
            if (piece_shapes[gs->piece.type][row][col])
                board_set(gs, (u8)(gs->piece.x + col), (u8)(gs->piece.y + row),
                          gs->piece.type + 1);
}
