#pragma once

#include <stdio.h>
#include <sokol_gfx.h>

typedef struct {
    sg_buffer vbuf;
    sg_shader shd;
    sg_pipeline pip;
} gfx_resources_t; 
