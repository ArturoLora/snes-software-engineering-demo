#ifndef BOARD_H
#define BOARD_H

#include "game_state.h"

void board_init(GameState *gs);
void board_clear(GameState *gs);
u8 board_get(GameState *gs, u8 x, u8 y);
void board_set(GameState *gs, u8 x, u8 y, u8 value);
u8 board_is_cell_occupied(GameState *gs, s8 x, s8 y);
u8 board_detect_full_lines(GameState *gs);
void board_collapse_lines(GameState *gs);

#endif
