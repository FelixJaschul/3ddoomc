#include <stdio.h>

#include "u_assert.h"
#include "u_map.h"

#include "../src/u_map.c"
#include "../src/u_log.c"

int main(int argc, char *argv[]) {
    {
        struct map m;
        map_init(
            &m,
            map_hash_str,
            NULL,
            NULL,
            map_dup_str,
            map_cmp_str,
            map_default_free,
            map_default_free,
            NULL);

        map_insert(&m, strdup("hello"), strdup("world"));

        ASSERT(map_size(&m) == 1);
        ASSERT(!strcmp(*map_find(const char*, &m, "hello"), "world"));
    }

    {
        struct map m;
        map_init(
            &m,
            map_hash_id,
            NULL,
            NULL,
            NULL,
            map_cmp_id,
            NULL,
            map_default_free,
            NULL);

        map_insert(&m, 32, strdup("world"));

        ASSERT(map_size(&m) == 1);
        ASSERT(!strcmp(*map_find(const char*, &m, 32), "world"));

        map_insert(&m, 32, strdup("not world"));

        ASSERT(map_size(&m) == 1);
        ASSERT(!strcmp(*map_find(const char*, &m, 32), "not world"));
    }
    return 0;
}
