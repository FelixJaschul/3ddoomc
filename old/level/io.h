#include <stdio.h>

#include "level/level_defs.h"

#define IO_VERSION ((int) 0x00000001)

// IO status
enum {
    IO_OK                   = 0,

    IO_BAD_START_MAGIC      = 1,
    IO_BAD_ARRAY_MAGIC      = 2,
    IO_BAD_END_MAGIC        = 3,

    IO_EXTRA_DATA           = 4,

    IO_BAD_INDEX            = 5,
    IO_BAD_TEXTURE          = 6,

    IO_UNKNOWN_VERSION = 8,
};

// read level from buffer
int io_load_level(level_t *level, const u8 *src, usize n);

// write level to file
int io_save_level(FILE *file, level_t *level);
