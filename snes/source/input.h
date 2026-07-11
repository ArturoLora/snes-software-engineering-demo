#ifndef INPUT_H
#define INPUT_H

#include <snes.h>

typedef struct
{
    u8 left;
    u8 right;
    u8 down;
} InputIntent;

InputIntent input_read(void);

#endif
