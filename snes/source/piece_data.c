#include "piece_data.h"

/* Rotation-0 (spawn) shape of each of the 7 piece types, adapted from
   GameInfo::tetraminos[7][4][4][4] (reference/apotris/source/tetromino.cpp:19-177).
   Only rotation index 0 is kept — no rotation system yet. Same type order as
   Apotris: 0=I, 1=J, 2=L, 3=O, 4=S, 5=T, 6=Z. */
const u8 piece_shapes[PIECE_TYPE_COUNT][PIECE_GRID_SIZE][PIECE_GRID_SIZE] = {
    { /* I */
        {0, 0, 0, 0},
        {1, 1, 1, 1},
        {0, 0, 0, 0},
        {0, 0, 0, 0}
    },
    { /* J */
        {1, 0, 0, 0},
        {1, 1, 1, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0}
    },
    { /* L */
        {0, 0, 1, 0},
        {1, 1, 1, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0}
    },
    { /* O */
        {0, 1, 1, 0},
        {0, 1, 1, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0}
    },
    { /* S */
        {0, 1, 1, 0},
        {1, 1, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0}
    },
    { /* T */
        {0, 1, 0, 0},
        {1, 1, 1, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0}
    },
    { /* Z */
        {1, 1, 0, 0},
        {0, 1, 1, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0}
    }
};
