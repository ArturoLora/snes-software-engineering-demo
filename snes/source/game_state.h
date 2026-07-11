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
    BoardState board;
} GameState;

#endif
