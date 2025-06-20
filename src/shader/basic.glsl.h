#pragma once
/*
    #version:1# (machine generated, don't edit!)

    Generated by sokol-shdc (https://github.com/floooh/sokol-tools)

    Cmdline: sokol-shdc --input src/shader/basic.glsl --output src/shader/basic.glsl.h --slang glsl300es:glsl330 --format=sokol_impl

    Overview:

        Shader program 'basic':
            Get shader desc: basic_basic_shader_desc(sg_query_backend());
            Vertex shader: vs
                Attribute slots:
                    ATTR_basic_vs_position = 0
                    ATTR_basic_vs_texcoord0 = 1
                Uniform block 'vs_params':
                    C struct: basic_vs_params_t
                    Bind slot: SLOT_basic_vs_params = 0
            Fragment shader: fs


    Shader descriptor structs:

        sg_shader basic = sg_make_shader(basic_basic_shader_desc(sg_query_backend()));

    Vertex attribute locations for vertex shader 'vs':

        sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
            .layout = {
                .attrs = {
                    [ATTR_basic_vs_position] = { ... },
                    [ATTR_basic_vs_texcoord0] = { ... },
                },
            },
            ...});

    Image bind slots, use as index in sg_bindings.vs_images[] or .fs_images[]


    Bind slot and C-struct for uniform block 'vs_params':

        basic_vs_params_t vs_params = {
            .model = ...;
            .view = ...;
            .proj = ...;
        };
        sg_apply_uniforms(SG_SHADERSTAGE_[VS|FS], SLOT_basic_vs_params, &SG_RANGE(vs_params));

*/
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>
#if !defined(SOKOL_GFX_INCLUDED)
  #error "Please include sokol_gfx.h before basic.glsl.h"
#endif
#if !defined(SOKOL_SHDC_ALIGN)
  #if defined(_MSC_VER)
    #define SOKOL_SHDC_ALIGN(a) __declspec(align(a))
  #else
    #define SOKOL_SHDC_ALIGN(a) __attribute__((aligned(a)))
  #endif
#endif
const sg_shader_desc* basic_basic_shader_desc(sg_backend backend);
#define ATTR_basic_vs_position (0)
#define ATTR_basic_vs_texcoord0 (1)
#define SLOT_basic_vs_params (0)
#pragma pack(push,1)
SOKOL_SHDC_ALIGN(16) typedef struct basic_vs_params_t {
    mat4s model;
    mat4s view;
    mat4s proj;
} basic_vs_params_t;
#pragma pack(pop)
#if defined(SOKOL_SHDC_IMPL)
/*
    #version 330
    
    uniform vec4 vs_params[12];
    layout(location = 0) in vec3 position;
    out vec2 uv;
    layout(location = 1) in vec2 texcoord0;
    
    void main()
    {
        gl_Position = ((mat4(vs_params[8], vs_params[9], vs_params[10], vs_params[11]) * mat4(vs_params[0], vs_params[1], vs_params[2], vs_params[3])) * mat4(vs_params[4], vs_params[5], vs_params[6], vs_params[7])) * vec4(position, 1.0);
        uv = texcoord0;
    }
    
*/
static const char basic_vs_source_glsl330[407] = {
    0x23,0x76,0x65,0x72,0x73,0x69,0x6f,0x6e,0x20,0x33,0x33,0x30,0x0a,0x0a,0x75,0x6e,
    0x69,0x66,0x6f,0x72,0x6d,0x20,0x76,0x65,0x63,0x34,0x20,0x76,0x73,0x5f,0x70,0x61,
    0x72,0x61,0x6d,0x73,0x5b,0x31,0x32,0x5d,0x3b,0x0a,0x6c,0x61,0x79,0x6f,0x75,0x74,
    0x28,0x6c,0x6f,0x63,0x61,0x74,0x69,0x6f,0x6e,0x20,0x3d,0x20,0x30,0x29,0x20,0x69,
    0x6e,0x20,0x76,0x65,0x63,0x33,0x20,0x70,0x6f,0x73,0x69,0x74,0x69,0x6f,0x6e,0x3b,
    0x0a,0x6f,0x75,0x74,0x20,0x76,0x65,0x63,0x32,0x20,0x75,0x76,0x3b,0x0a,0x6c,0x61,
    0x79,0x6f,0x75,0x74,0x28,0x6c,0x6f,0x63,0x61,0x74,0x69,0x6f,0x6e,0x20,0x3d,0x20,
    0x31,0x29,0x20,0x69,0x6e,0x20,0x76,0x65,0x63,0x32,0x20,0x74,0x65,0x78,0x63,0x6f,
    0x6f,0x72,0x64,0x30,0x3b,0x0a,0x0a,0x76,0x6f,0x69,0x64,0x20,0x6d,0x61,0x69,0x6e,
    0x28,0x29,0x0a,0x7b,0x0a,0x20,0x20,0x20,0x20,0x67,0x6c,0x5f,0x50,0x6f,0x73,0x69,
    0x74,0x69,0x6f,0x6e,0x20,0x3d,0x20,0x28,0x28,0x6d,0x61,0x74,0x34,0x28,0x76,0x73,
    0x5f,0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,0x38,0x5d,0x2c,0x20,0x76,0x73,0x5f,0x70,
    0x61,0x72,0x61,0x6d,0x73,0x5b,0x39,0x5d,0x2c,0x20,0x76,0x73,0x5f,0x70,0x61,0x72,
    0x61,0x6d,0x73,0x5b,0x31,0x30,0x5d,0x2c,0x20,0x76,0x73,0x5f,0x70,0x61,0x72,0x61,
    0x6d,0x73,0x5b,0x31,0x31,0x5d,0x29,0x20,0x2a,0x20,0x6d,0x61,0x74,0x34,0x28,0x76,
    0x73,0x5f,0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,0x30,0x5d,0x2c,0x20,0x76,0x73,0x5f,
    0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,0x31,0x5d,0x2c,0x20,0x76,0x73,0x5f,0x70,0x61,
    0x72,0x61,0x6d,0x73,0x5b,0x32,0x5d,0x2c,0x20,0x76,0x73,0x5f,0x70,0x61,0x72,0x61,
    0x6d,0x73,0x5b,0x33,0x5d,0x29,0x29,0x20,0x2a,0x20,0x6d,0x61,0x74,0x34,0x28,0x76,
    0x73,0x5f,0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,0x34,0x5d,0x2c,0x20,0x76,0x73,0x5f,
    0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,0x35,0x5d,0x2c,0x20,0x76,0x73,0x5f,0x70,0x61,
    0x72,0x61,0x6d,0x73,0x5b,0x36,0x5d,0x2c,0x20,0x76,0x73,0x5f,0x70,0x61,0x72,0x61,
    0x6d,0x73,0x5b,0x37,0x5d,0x29,0x29,0x20,0x2a,0x20,0x76,0x65,0x63,0x34,0x28,0x70,
    0x6f,0x73,0x69,0x74,0x69,0x6f,0x6e,0x2c,0x20,0x31,0x2e,0x30,0x29,0x3b,0x0a,0x20,
    0x20,0x20,0x20,0x75,0x76,0x20,0x3d,0x20,0x74,0x65,0x78,0x63,0x6f,0x6f,0x72,0x64,
    0x30,0x3b,0x0a,0x7d,0x0a,0x0a,0x00,
};
/*
    #version 330
    
    layout(location = 0) out vec4 frag_color;
    in vec2 uv;
    
    void main()
    {
        frag_color = vec4(uv, 0.0, 1.0);
    }
    
*/
static const char basic_fs_source_glsl330[124] = {
    0x23,0x76,0x65,0x72,0x73,0x69,0x6f,0x6e,0x20,0x33,0x33,0x30,0x0a,0x0a,0x6c,0x61,
    0x79,0x6f,0x75,0x74,0x28,0x6c,0x6f,0x63,0x61,0x74,0x69,0x6f,0x6e,0x20,0x3d,0x20,
    0x30,0x29,0x20,0x6f,0x75,0x74,0x20,0x76,0x65,0x63,0x34,0x20,0x66,0x72,0x61,0x67,
    0x5f,0x63,0x6f,0x6c,0x6f,0x72,0x3b,0x0a,0x69,0x6e,0x20,0x76,0x65,0x63,0x32,0x20,
    0x75,0x76,0x3b,0x0a,0x0a,0x76,0x6f,0x69,0x64,0x20,0x6d,0x61,0x69,0x6e,0x28,0x29,
    0x0a,0x7b,0x0a,0x20,0x20,0x20,0x20,0x66,0x72,0x61,0x67,0x5f,0x63,0x6f,0x6c,0x6f,
    0x72,0x20,0x3d,0x20,0x76,0x65,0x63,0x34,0x28,0x75,0x76,0x2c,0x20,0x30,0x2e,0x30,
    0x2c,0x20,0x31,0x2e,0x30,0x29,0x3b,0x0a,0x7d,0x0a,0x0a,0x00,
};
/*
    #version 300 es
    
    uniform vec4 vs_params[12];
    layout(location = 0) in vec3 position;
    out vec2 uv;
    layout(location = 1) in vec2 texcoord0;
    
    void main()
    {
        gl_Position = ((mat4(vs_params[8], vs_params[9], vs_params[10], vs_params[11]) * mat4(vs_params[0], vs_params[1], vs_params[2], vs_params[3])) * mat4(vs_params[4], vs_params[5], vs_params[6], vs_params[7])) * vec4(position, 1.0);
        uv = texcoord0;
    }
    
*/
static const char basic_vs_source_glsl300es[410] = {
    0x23,0x76,0x65,0x72,0x73,0x69,0x6f,0x6e,0x20,0x33,0x30,0x30,0x20,0x65,0x73,0x0a,
    0x0a,0x75,0x6e,0x69,0x66,0x6f,0x72,0x6d,0x20,0x76,0x65,0x63,0x34,0x20,0x76,0x73,
    0x5f,0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,0x31,0x32,0x5d,0x3b,0x0a,0x6c,0x61,0x79,
    0x6f,0x75,0x74,0x28,0x6c,0x6f,0x63,0x61,0x74,0x69,0x6f,0x6e,0x20,0x3d,0x20,0x30,
    0x29,0x20,0x69,0x6e,0x20,0x76,0x65,0x63,0x33,0x20,0x70,0x6f,0x73,0x69,0x74,0x69,
    0x6f,0x6e,0x3b,0x0a,0x6f,0x75,0x74,0x20,0x76,0x65,0x63,0x32,0x20,0x75,0x76,0x3b,
    0x0a,0x6c,0x61,0x79,0x6f,0x75,0x74,0x28,0x6c,0x6f,0x63,0x61,0x74,0x69,0x6f,0x6e,
    0x20,0x3d,0x20,0x31,0x29,0x20,0x69,0x6e,0x20,0x76,0x65,0x63,0x32,0x20,0x74,0x65,
    0x78,0x63,0x6f,0x6f,0x72,0x64,0x30,0x3b,0x0a,0x0a,0x76,0x6f,0x69,0x64,0x20,0x6d,
    0x61,0x69,0x6e,0x28,0x29,0x0a,0x7b,0x0a,0x20,0x20,0x20,0x20,0x67,0x6c,0x5f,0x50,
    0x6f,0x73,0x69,0x74,0x69,0x6f,0x6e,0x20,0x3d,0x20,0x28,0x28,0x6d,0x61,0x74,0x34,
    0x28,0x76,0x73,0x5f,0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,0x38,0x5d,0x2c,0x20,0x76,
    0x73,0x5f,0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,0x39,0x5d,0x2c,0x20,0x76,0x73,0x5f,
    0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,0x31,0x30,0x5d,0x2c,0x20,0x76,0x73,0x5f,0x70,
    0x61,0x72,0x61,0x6d,0x73,0x5b,0x31,0x31,0x5d,0x29,0x20,0x2a,0x20,0x6d,0x61,0x74,
    0x34,0x28,0x76,0x73,0x5f,0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,0x30,0x5d,0x2c,0x20,
    0x76,0x73,0x5f,0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,0x31,0x5d,0x2c,0x20,0x76,0x73,
    0x5f,0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,0x32,0x5d,0x2c,0x20,0x76,0x73,0x5f,0x70,
    0x61,0x72,0x61,0x6d,0x73,0x5b,0x33,0x5d,0x29,0x29,0x20,0x2a,0x20,0x6d,0x61,0x74,
    0x34,0x28,0x76,0x73,0x5f,0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,0x34,0x5d,0x2c,0x20,
    0x76,0x73,0x5f,0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,0x35,0x5d,0x2c,0x20,0x76,0x73,
    0x5f,0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,0x36,0x5d,0x2c,0x20,0x76,0x73,0x5f,0x70,
    0x61,0x72,0x61,0x6d,0x73,0x5b,0x37,0x5d,0x29,0x29,0x20,0x2a,0x20,0x76,0x65,0x63,
    0x34,0x28,0x70,0x6f,0x73,0x69,0x74,0x69,0x6f,0x6e,0x2c,0x20,0x31,0x2e,0x30,0x29,
    0x3b,0x0a,0x20,0x20,0x20,0x20,0x75,0x76,0x20,0x3d,0x20,0x74,0x65,0x78,0x63,0x6f,
    0x6f,0x72,0x64,0x30,0x3b,0x0a,0x7d,0x0a,0x0a,0x00,
};
/*
    #version 300 es
    precision mediump float;
    precision highp int;
    
    layout(location = 0) out highp vec4 frag_color;
    in highp vec2 uv;
    
    void main()
    {
        frag_color = vec4(uv, 0.0, 1.0);
    }
    
*/
static const char basic_fs_source_glsl300es[185] = {
    0x23,0x76,0x65,0x72,0x73,0x69,0x6f,0x6e,0x20,0x33,0x30,0x30,0x20,0x65,0x73,0x0a,
    0x70,0x72,0x65,0x63,0x69,0x73,0x69,0x6f,0x6e,0x20,0x6d,0x65,0x64,0x69,0x75,0x6d,
    0x70,0x20,0x66,0x6c,0x6f,0x61,0x74,0x3b,0x0a,0x70,0x72,0x65,0x63,0x69,0x73,0x69,
    0x6f,0x6e,0x20,0x68,0x69,0x67,0x68,0x70,0x20,0x69,0x6e,0x74,0x3b,0x0a,0x0a,0x6c,
    0x61,0x79,0x6f,0x75,0x74,0x28,0x6c,0x6f,0x63,0x61,0x74,0x69,0x6f,0x6e,0x20,0x3d,
    0x20,0x30,0x29,0x20,0x6f,0x75,0x74,0x20,0x68,0x69,0x67,0x68,0x70,0x20,0x76,0x65,
    0x63,0x34,0x20,0x66,0x72,0x61,0x67,0x5f,0x63,0x6f,0x6c,0x6f,0x72,0x3b,0x0a,0x69,
    0x6e,0x20,0x68,0x69,0x67,0x68,0x70,0x20,0x76,0x65,0x63,0x32,0x20,0x75,0x76,0x3b,
    0x0a,0x0a,0x76,0x6f,0x69,0x64,0x20,0x6d,0x61,0x69,0x6e,0x28,0x29,0x0a,0x7b,0x0a,
    0x20,0x20,0x20,0x20,0x66,0x72,0x61,0x67,0x5f,0x63,0x6f,0x6c,0x6f,0x72,0x20,0x3d,
    0x20,0x76,0x65,0x63,0x34,0x28,0x75,0x76,0x2c,0x20,0x30,0x2e,0x30,0x2c,0x20,0x31,
    0x2e,0x30,0x29,0x3b,0x0a,0x7d,0x0a,0x0a,0x00,
};
const sg_shader_desc* basic_basic_shader_desc(sg_backend backend) {
  if (backend == SG_BACKEND_GLCORE33) {
    static sg_shader_desc desc;
    static bool valid;
    if (!valid) {
      valid = true;
      desc.attrs[0].name = "position";
      desc.attrs[1].name = "texcoord0";
      desc.vs.source = basic_vs_source_glsl330;
      desc.vs.entry = "main";
      desc.vs.uniform_blocks[0].size = 192;
      desc.vs.uniform_blocks[0].layout = SG_UNIFORMLAYOUT_STD140;
      desc.vs.uniform_blocks[0].uniforms[0].name = "vs_params";
      desc.vs.uniform_blocks[0].uniforms[0].type = SG_UNIFORMTYPE_FLOAT4;
      desc.vs.uniform_blocks[0].uniforms[0].array_count = 12;
      desc.fs.source = basic_fs_source_glsl330;
      desc.fs.entry = "main";
      desc.label = "basic_basic_shader";
    }
    return &desc;
  }
  if (backend == SG_BACKEND_GLES3) {
    static sg_shader_desc desc;
    static bool valid;
    if (!valid) {
      valid = true;
      desc.attrs[0].name = "position";
      desc.attrs[1].name = "texcoord0";
      desc.vs.source = basic_vs_source_glsl300es;
      desc.vs.entry = "main";
      desc.vs.uniform_blocks[0].size = 192;
      desc.vs.uniform_blocks[0].layout = SG_UNIFORMLAYOUT_STD140;
      desc.vs.uniform_blocks[0].uniforms[0].name = "vs_params";
      desc.vs.uniform_blocks[0].uniforms[0].type = SG_UNIFORMTYPE_FLOAT4;
      desc.vs.uniform_blocks[0].uniforms[0].array_count = 12;
      desc.fs.source = basic_fs_source_glsl300es;
      desc.fs.entry = "main";
      desc.label = "basic_basic_shader";
    }
    return &desc;
  }
  return 0;
}
#endif /* SOKOL_SHDC_IMPL */
