#ifndef SHARED_DEFS_H
#define SHARED_DEFS_H

// defs shared between shaders and C files

#define SIDE_SEGMENT_BOTTOM 0
#define SIDE_SEGMENT_MIDDLE 1
#define SIDE_SEGMENT_TOP 2
#define SIDE_SEGMENT_WALL 3

// level element types as flat indices
#define T_VERTEX_INDEX  0
#define T_WALL_INDEX    1
#define T_SIDE_INDEX    2
#define T_SIDEMAT_INDEX 3
#define T_SECTOR_INDEX  4
#define T_SECTMAT_INDEX 5
#define T_DECAL_INDEX   6
#define T_OBJECT_INDEX  7
#define T_COUNT         8

// level element types (T_*) as bit flags
#define T_VERTEX    (1 << 0)
#define T_WALL      (1 << 1)
#define T_SIDE      (1 << 2)
#define T_SIDEMAT   (1 << 3)
#define T_SECTOR    (1 << 4)
#define T_SECTMAT   (1 << 5)
#define T_DECAL     (1 << 6)
#define T_OBJECT    (1 << 7)

#define STF_NONE    (0)
#define STF_BOT_ABS (1 << 0)
#define STF_TOP_ABS (1 << 1)
#define STF_Y_ABS   (1 << 2)
#define STF_EZPORT  (1 << 3)
#define STF_UNPEG   (1 << 4)

#define PLANE_TYPE_FLOOR 0
#define PLANE_TYPE_CEIL 1

#endif
