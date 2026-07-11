#include <snes.h>
#include "render.h"
#include "piece_data.h"

/* Story 4.1: renders the active piece as up to 4 OAM sprites, one per
   occupied cell of piece_shapes[type] (rotation-0 shape, Story 3.2/3.7).
   No Apotris algorithm is adapted here - Apotris runs on GBA's own sprite
   hardware, not portable to PVSnesLib/SNES OAM (game-architecture.md#8).
   render.c is the only module including video headers and is read-only over
   GameState - it never writes gs->piece/board (game-architecture.md#2, #7). */

#define PIECE_SPRITE_COUNT 4

/* Root cause (confirmed against the official PVSnesLib wiki,
   https://github.com/alekmaul/pvsneslib/wiki/Sprites): the `id` argument to
   oamSet()/oamSetEx()/oamSetVisible() is an OAM byte offset, not a plain
   sprite index - each sprite occupies 4 bytes, so sprite 0/1/2/3 must be
   passed as id 0/4/8/12. Passing raw 0/1/2/3 works by coincidence for
   sprite 0 (0*4==0) but corrupts adjacent OAM entries for sprite 1+. */
#define OAM_ID_STEP 4

extern char piecetiles, piecetiles_end;
extern char piecepal, piecepal_end;

void render_init(void)
{
    oamInitGfxSet((u8 *)&piecetiles, (u16)(&piecetiles_end - &piecetiles),
                  (u8 *)&piecepal, (u16)(&piecepal_end - &piecepal),
                  0, 0x0000, OBJ_SIZE8_L16);
}

/* BoardState has 22 rows: 20 visible + 2 top buffer rows reserved for
   spawn/top-out (game-architecture.md#4). The playfield BG (Story 1.2) shows
   only those 20 visible rows, anchored to the screen's top-left corner, so a
   board row needs 2 subtracted before becoming a screen pixel row - skipping
   this offset would draw the piece 16px below where the playfield expects
   it. */
void render_sync_piece(GameState *gs, u8 active)
{
    u8 row, col, slot;

    if (!active)
    {
        for (slot = 0; slot < PIECE_SPRITE_COUNT; slot++)
        {
            oamSetEx(slot * OAM_ID_STEP, OBJ_SMALL, OBJ_HIDE);
            oamSetVisible(slot * OAM_ID_STEP, OBJ_HIDE);
        }
        return;
    }

    slot = 0;
    for (row = 0; row < PIECE_GRID_SIZE; row++)
        for (col = 0; col < PIECE_GRID_SIZE; col++)
            if (piece_shapes[gs->piece.type][row][col])
            {
                u16 screen_x = (u16)((gs->piece.x + col) * 8);
                u16 screen_y = (u16)((gs->piece.y + row - 2) * 8);
                u16 id = slot * OAM_ID_STEP;

                oamSet(id, screen_x, screen_y, 3, 0, 0, 0, 0);
                oamSetEx(id, OBJ_SMALL, OBJ_SHOW);
                oamSetVisible(id, OBJ_SHOW);
                slot++;
            }
}
