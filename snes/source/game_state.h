#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <snes.h>

#define BOARD_WIDTH 10
#define BOARD_HEIGHT 22

typedef struct
{
    u8 board[BOARD_HEIGHT][BOARD_WIDTH];
} BoardState;

typedef struct
{
    u8 type;
    u8 rotation;
    s8 x;
    s8 y;
} ActivePiece;

#define LINES_TO_CLEAR_MAX 4

typedef struct
{
    u8 rows[LINES_TO_CLEAR_MAX];
    u8 count;
} LinesToClear;

/* Story 3.8 - 7-bag. bag[] holds the types still pending in the current bag;
   bag_count is how many are left, and they are consumed from the end.
   Kept equal to piece_data.h's PIECE_TYPE_COUNT without including it, so
   game_state.h stays dependent on nothing but snestypes
   (game-architecture.md#7). queue.c asserts the two agree. */
#define PIECE_QUEUE_BAG_SIZE 7

typedef struct
{
    u8 bag[PIECE_QUEUE_BAG_SIZE];
    u8 bag_count;
} PieceQueue;

/* Story 3.8: PieceQueue goes LAST, after lines, on purpose. The V1 Lua script
   reads piece/lines at fixed byte offsets inside GameState; inserting a field
   before them would shift those offsets and the harness would report PASS
   while measuring the wrong bytes - finding G1 of Story 4-8. */
typedef struct
{
    BoardState board;
    ActivePiece piece;
    LinesToClear lines;
    PieceQueue queue;
} GameState;

#endif
