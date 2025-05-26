#ifndef CONFIG_H
#define CONFIG_H

#define TARGET_WIDTH 448
#define TARGET_HEIGHT 252

#define TARGET_3D_WIDTH 768
#define TARGET_3D_HEIGHT 432

/* #define TARGET_WIDTH 1280 */
/* #define TARGET_HEIGHT 720 */

#define TICKS_PER_SECOND 60
#define NS_PER_TICK (1000000000 / TICKS_PER_SECOND)
#define MS_PER_TICK (1000 / TICKS_PER_SECOND)
#define TICK_DT (1.0f / (f32) TICKS_PER_SECOND)

#define TAG_NONE 0
#define TAG_MAX 4096

#define BLOCK_SIZE 8

#define LIGHT_MAX 31
#define LIGHT_EXTRA 35

#define MAPEDITOR

// TODO
#define RELOADABLE

#define TEXTURE_NOTEX "NOTEX"

#define PX_PER_UNIT 32

#define SPRITES_BASE_DIR "res"
#define SOUNDS_BASE_DIR "res"

#endif // ifndef CONFIG_H
