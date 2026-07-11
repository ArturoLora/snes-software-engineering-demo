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

typedef struct
{
    BoardState board;
    ActivePiece piece;
    LinesToClear lines;
} GameState;

#endif
