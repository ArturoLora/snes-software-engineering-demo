#ifndef PIECE_H
#define PIECE_H

#include "game_state.h"

void piece_spawn(GameState *gs);
void piece_move_left(GameState *gs);
void piece_move_right(GameState *gs);
void piece_apply_gravity(GameState *gs);
void piece_lock(GameState *gs);

#endif
