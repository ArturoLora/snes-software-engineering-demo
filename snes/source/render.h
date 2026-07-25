#ifndef RENDER_H
#define RENDER_H

#include "game_state.h"

void render_init(void);
void render_sync_piece(GameState *gs, u8 active);

/* Story 4.8 - playfield tilemap: WRAM mirror + DMA on lock. */
void render_board_init(void);
void render_sync_board(GameState *gs);
void render_flush_board(void);

#endif
