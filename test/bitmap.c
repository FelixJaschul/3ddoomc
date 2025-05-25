#include <stdio.h>

#include "u_assert.h"
#include "u_bitmap.h"

#include "../src/u_log.c"

int main(int argc, char *argv[]) {
    {
        BITMAP_DECL(bits, 11);

        memset(bits, 0, sizeof(bits));
        bits[1] = 0xF8;

        int res;
        ASSERT((res = bitmap_count(bits, 11, true)) == 0, "%d", res);
        ASSERT((res = bitmap_find(bits, 11, 0, true)) == INT_MAX, "%d", res);
        ASSERT((res = bitmap_count(bits, 11, false)) == 11, "%d", res);
        ASSERT((res = bitmap_find(bits, 11, 0, false)) == 0, "%d", res);
    }

    {
        BITMAP_DECL(bits, 3847);

        memset(bits, 0, sizeof(bits));
        bits[sizeof(bits) - 1] = 0x80;

        int res;
        ASSERT((res = bitmap_count(bits, 3847, true)) == 0, "%d", res);
        ASSERT((res = bitmap_find(bits, 3847, 0, true)) == INT_MAX, "%d", res);
        ASSERT((res = bitmap_count(bits, 3847, false)) == 3847, "%d", res);
        ASSERT((res = bitmap_find(bits, 3847, 0, false)) == 0, "%d", res);

        bitmap_set(bits, 475);
        ASSERT((res = bitmap_find(bits, 3847, 0, true)) == 475, "%d", res);
        bitmap_set(bits, 470);
        ASSERT((res = bitmap_find(bits, 3847, 0, true)) == 470, "%d", res);
        ASSERT((res = bitmap_find(bits, 3847, 476, true)) == INT_MAX, "%d", res);
        ASSERT((res = bitmap_find(bits, 3847, 475, true)) == 475, "%d", res);
        ASSERT((res = bitmap_find(bits, 3847, 474, true)) == 475, "%d", res);
        ASSERT((res = bitmap_find(bits, 3847, 470, true)) == 470, "%d", res);
        ASSERT((res = bitmap_find(bits, 3847, 469, true)) == 470, "%d", res);
        bitmap_clr(bits, 470);
        ASSERT((res = bitmap_find(bits, 3847, 0, true)) == 475, "%d", res);

        ASSERT((res = bitmap_find(bits, 3847, 0, false)) == 0, "%d", res);
        bitmap_set(bits, 0);
        ASSERT((res = bitmap_find(bits, 3847, 0, false)) == 1, "%d", res);
        ASSERT((res = bitmap_find(bits, 3847, 0, true)) == 0, "%d", res);

        bitmap_fill(bits, 3847, 0);
        bitmap_set(bits, 3846);
        ASSERT((res = bitmap_find(bits, 3847, 0, true)) == 3846, "%d", res);
    }

    return 0;
}
