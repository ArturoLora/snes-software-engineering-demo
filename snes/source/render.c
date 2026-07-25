#include <snes.h>
#include "render.h"
#include "piece_data.h"
#include "board.h"

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

/* Story 4.8: WRAM mirror of the playfield tilemap.

   background.h exposes no single-cell write - none of its 12 functions do
   (investigations/pvsneslib-tilemap-runtime-update-investigation.md). The
   confirmed pattern, from the official games/breakout/breakout.c example, is
   to keep the tilemap mirrored in WRAM, mutate it on the low-frequency event,
   and push it with dmaCopyVram() right after WaitForVBlank().
   game-architecture.md#8 decided to transfer the whole playfield tilemap per
   event rather than track dirty regions - at this size that would be
   complexity with no measurable payoff. */
#define NAMETABLE_WIDTH 32
#define NAMETABLE_HEIGHT 32
#define PLAYFIELD_MAP_ENTRIES (NAMETABLE_WIDTH * NAMETABLE_HEIGHT)
#define PLAYFIELD_MAP_MAX_BYTES (PLAYFIELD_MAP_ENTRIES * 2)

/* Same VRAM word address bgInitMapSet() already receives in main.c. */
#define PLAYFIELD_MAP_VRAM 0x7000

/* Board row -> nametable row. Identical reasoning to render_sync_piece()
   below: -2 for the board's top buffer rows, +3 for Story 4.7's centering
   shift. Net +1. Board rows 2-21 land on nametable rows 3-22, which is
   exactly where the playfield art sits. */
#define BOARD_ROW_TO_NAMETABLE 1

/* Where Story 4.8 painted the solid accent cell in playfield.png: nametable
   row 28, column 0. The visible area is 28 rows, the SC_32x32 nametable has
   32, so rows 28-31 exist in VRAM and are never drawn - the tile gets into
   the tileset without showing up on screen.

   The locked-cell tile INDEX is never written as a literal: it is read back
   from the generated map at this coordinate. If gfx4snes dedupes differently
   one day and renumbers the tiles, this still works. A hardcoded index would
   break silently, and the symptom would be a board drawn with the wrong
   tile - exactly the kind of failure V0 cannot catch. */
#define LOCKED_TILE_COL 0
#define LOCKED_TILE_ROW 28

extern char playfieldmap, playfieldmap_end;

static u16 board_map[PLAYFIELD_MAP_ENTRIES];
static u16 locked_entry;
static u16 map_bytes;
static u8 board_dirty;

/* Seeds the mirror from the generated map in ROM rather than synthesising a
   background, so the art from Stories 4.4/4.5/4.7 is preserved without this
   code knowing anything about it. Must run after bgInitMapSet() and before
   setScreenOn(), same as render_init(). */
void render_board_init(void)
{
    u8 *rom = (u8 *)&playfieldmap;
    u8 *dst = (u8 *)board_map;
    u16 bytes = (u16)(&playfieldmap_end - &playfieldmap);
    u16 i;

    if (bytes > PLAYFIELD_MAP_MAX_BYTES)
        bytes = PLAYFIELD_MAP_MAX_BYTES;

    for (i = 0; i < bytes; i++)
        dst[i] = rom[i];

    map_bytes = bytes;
    locked_entry =
        board_map[(u16)LOCKED_TILE_ROW * NAMETABLE_WIDTH + LOCKED_TILE_COL];
    board_dirty = 0;
}

/* Bakes the logical board into the mirror: every occupied cell gets the
   locked-cell entry, every empty one goes back to the entry the asset
   defines. Read-only over GameState (game-architecture.md#7) - board_get()
   is board.c's public accessor, and board.c stays the owner of board[][].
   Marks the mirror pending; the transfer itself happens in
   render_flush_board(). */
void render_sync_board(GameState *gs)
{
    u16 *rom = (u16 *)&playfieldmap;
    u8 x, y;

    for (y = 0; y < BOARD_HEIGHT; y++)
    {
        u16 nrow = (u16)y + BOARD_ROW_TO_NAMETABLE;

        for (x = 0; x < BOARD_WIDTH; x++)
        {
            u16 idx = nrow * NAMETABLE_WIDTH + (u16)x;

            board_map[idx] = board_get(gs, x, y) ? locked_entry : rom[idx];
        }
    }

    board_dirty = 1;
}

/* Called at the top of the frame loop, i.e. immediately after the previous
   iteration's WaitForVBlank(). No-op unless a lock marked the mirror pending,
   so no VRAM transfer happens for movement or gravity - those only move
   sprites (game-architecture.md#8). */
void render_flush_board(void)
{
    if (!board_dirty)
        return;

    dmaCopyVram((u8 *)board_map, PLAYFIELD_MAP_VRAM, map_bytes);
    board_dirty = 0;
}

void render_init(void)
{
    oamInitGfxSet((u8 *)&piecetiles, (u16)(&piecetiles_end - &piecetiles),
                  (u8 *)&piecepal, (u16)(&piecepal_end - &piecepal),
                  0, 0x0000, OBJ_SIZE8_L16);
}

/* BoardState has 22 rows: 20 visible + 2 top buffer rows reserved for
   spawn/top-out (game-architecture.md#4). A board row needs 2 subtracted to
   become a playfield-relative screen row. Story 4.7 shifted the playfield BG
   itself down 3 rows (to center the playfield+debug layout on screen,
   snes/playfield.png), so the net conversion is -2+3 = +1 - skipping this
   offset would draw the piece 24px above where the playfield BG now sits. */
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
                u16 screen_y = (u16)((gs->piece.y + row + 1) * 8);
                u16 id = slot * OAM_ID_STEP;

                oamSet(id, screen_x, screen_y, 3, 0, 0, 0, 0);
                oamSetEx(id, OBJ_SMALL, OBJ_SHOW);
                oamSetVisible(id, OBJ_SHOW);
                slot++;
            }
}
