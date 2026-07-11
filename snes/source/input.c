#include "input.h"

/* Story 4.2: sole boundary with PVSnesLib pad symbols (game-architecture.md
   #2, #6) - no other gameplay module (board.c/piece.c/piece_data.c) includes
   pad constants. padsDown() (pad_keysdown, edge-triggered) is used instead
   of padsCurrent() (pad_keys, held) so movement is one step per press, no
   DAS/ARR - matching Story 3.3/3.4. */
InputIntent input_read(void)
{
    InputIntent intent;
    u16 keys = padsDown(0);

    intent.left = (keys & KEY_LEFT) ? 1 : 0;
    intent.right = (keys & KEY_RIGHT) ? 1 : 0;
    intent.down = (keys & KEY_DOWN) ? 1 : 0;

    return intent;
}
