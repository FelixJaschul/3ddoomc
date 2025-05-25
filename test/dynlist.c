#include "u_assert.h"

#include "u_dynlist.h"
#include "../src/u_dynlist.c"

#include "u_log.h"
#include "../src/u_log.c"

int main(int argc, char *argv[]) {
    // basics
    {
        int *list = NULL;
        ASSERT(dynlist_size(list) == 0);
        ASSERT(dynlist_capacity(list) == 0);

        *dynlist_push(list) = 4;
        ASSERT(list[0] == 4);
        ASSERT(dynlist_size(list) == 1);
        ASSERT(dynlist_capacity(list) >= 1);

        ASSERT(dynlist_remove(list, 0) == 4);

        *dynlist_push(list) = 4;
        *dynlist_push(list) = 5;
        *dynlist_push(list) = 6;

        ASSERT(list[0] == 4);
        ASSERT(list[1] == 5);
        ASSERT(list[2] == 6);

        *dynlist_insert(list, 1) = 12;
        ASSERT(dynlist_size(list) == 4);
        ASSERT(list[0] == 4);
        ASSERT(list[1] == 12);
        ASSERT(list[2] == 5);
        ASSERT(list[3] == 6);

        ASSERT(dynlist_remove(list, 3) == 6);
        ASSERT(dynlist_size(list) == 3);
        ASSERT(list[0] == 4);
        ASSERT(list[1] == 12);
        ASSERT(list[2] == 5);

        ASSERT(dynlist_remove(list, 1) == 12);
        ASSERT(dynlist_size(list) == 2);
        ASSERT(list[0] == 4);
        ASSERT(list[1] == 5);

        *dynlist_prepend(list) = 45;
        ASSERT(list[0] == 45);
        ASSERT(dynlist_size(list) == 3);
        ASSERT(list[0] == 45);
        ASSERT(list[2] == 5);
    }

    // each
    {
        int *list = NULL;
        dynlist_each(list, it) { ASSERT(false); }

        for (int i = 0; i < 256; i++) {
            *dynlist_push(list) = i * i;
        }

        dynlist_each(list, it) {
            ASSERT(*it.el == it.i * it.i);
        }

        for (int i = 0; i < 64; i++) {
            dynlist_remove(list, 0);
        }

        ASSERT(dynlist_size(list) == 256 - 64);

        for (int i = 0; i < 256 - 64; i++) {
            ASSERT(list[i] == (i + 64) * (i + 64));
        }
    }

    // alloc/free
    {
        int *list = NULL;
        ASSERT(!list);

        dynlist_free(list);
        ASSERT(!list);

        dynlist_alloc(list);
        ASSERT(list);
        ASSERT(dynlist_capacity(list) == DYNLIST_INIT_CAP);

        dynlist_free(list);
        ASSERT(!list);
    }

    // ensure
    {
        int *list = NULL, *oldlist;

        dynlist_ensure(list, 1024);
        oldlist = list;

        ASSERT(list);
        ASSERT(dynlist_size(list) == 0);
        ASSERT(dynlist_capacity(list) >= 1024);

        for (int i = 0; i < 1024; i++) {
            *dynlist_push(list) = i << 1;
        }

        ASSERT(list == oldlist);
        ASSERT(dynlist_size(list) == 1024);
    }
    return 0;
}
