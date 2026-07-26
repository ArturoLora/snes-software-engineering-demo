#ifndef QUEUE_H
#define QUEUE_H

#include "game_state.h"

/* Story 3.8 - 7-bag piece queue. */
void queue_init(GameState *gs);
u8 queue_next(GameState *gs);

#endif
