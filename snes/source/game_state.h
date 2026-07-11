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

typedef struct
{
    BoardState board;
    ActivePiece piece;
} GameState;

#endif
