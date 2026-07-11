#ifndef RENDER_H
#define RENDER_H

#include "game_state.h"

void render_init(void);
void render_sync_piece(GameState *gs, u8 active);

#endif
