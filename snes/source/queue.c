#include <snes.h>
#include "queue.h"
#include "piece_data.h"

/* Story 3.8: 7-bag randomiser.

   Adapted from Apotris Game::fillBag()/nextPiece()
   (reference/apotris/source/tetrisEngine.cpp): fill the bag with one of each
   type, shuffle it, hand out pieces until it is empty, refill. Apotris keeps a
   std::list of upcoming pieces to feed its next-piece preview; there is no
   preview here, so a single bag with a count is enough and no list is needed.

   No video or input headers: this is gameplay logic (game-architecture.md#7).

   PVSnesLib exposes rand() but NOT srand() - checked in
   $PVSNESLIB_HOME/pvsneslib/include/. The generator therefore produces the
   same stream after every reset, and the only way to vary the sequence is to
   consume a different NUMBER of values before each shuffle. Two things do
   that: main.c burns one rand() per frame, and the bag is refilled lazily, at
   the moment a piece is actually requested. From the second bag onwards the
   refill lands on a frame that depends on when the previous piece locked,
   which depends on the player.

   Known limit, deferred: the FIRST bag is filled before the player can touch
   anything, so the first seven pieces are the same on every run. Closing that
   needs a real entropy source - seeding from snes_vblank_count on a title
   screen at first button press - and there is no title screen yet. */

#if PIECE_QUEUE_BAG_SIZE != PIECE_TYPE_COUNT
#error "PIECE_QUEUE_BAG_SIZE must match PIECE_TYPE_COUNT"
#endif

static void bag_refill(GameState *gs)
{
    u8 i, j;
    u8 swap;

    for (i = 0; i < PIECE_QUEUE_BAG_SIZE; i++)
        gs->queue.bag[i] = i;

    /* Fisher-Yates, walking down so the modulo range shrinks with the index.
       rand() is u16; the modulo bias at these sizes is far below anything a
       player could perceive, and the bag guarantees fairness regardless -
       every type still comes out exactly once. */
    for (i = PIECE_QUEUE_BAG_SIZE - 1; i > 0; i--)
    {
        j = (u8)(rand() % (u16)(i + 1));
        swap = gs->queue.bag[i];
        gs->queue.bag[i] = gs->queue.bag[j];
        gs->queue.bag[j] = swap;
    }

    gs->queue.bag_count = PIECE_QUEUE_BAG_SIZE;
}

/* Leaves the bag empty. The first refill happens on the first queue_next(),
   not here, so it consumes the RNG as late as possible. */
void queue_init(GameState *gs)
{
    gs->queue.bag_count = 0;
}

u8 queue_next(GameState *gs)
{
    if (gs->queue.bag_count == 0)
        bag_refill(gs);

    gs->queue.bag_count--;
    return gs->queue.bag[gs->queue.bag_count];
}
