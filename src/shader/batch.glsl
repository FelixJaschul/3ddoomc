@module batch

@vs vs
in vec2 a_position;
in vec2 a_texcoord0;

in vec2 a_offset;
in vec2 a_scale;
in vec2 a_uvmin;
in vec2 a_uvmax;
in float a_layer;
in vec4 a_color;
in float a_z;
in float a_flags;

uniform vs_params {
	mat4 model;
    mat4 view;
    mat4 proj;
};

flat out int layer;
out vec2 uv;
out vec4 color;
out float depth;

// must match with gfx.h
#define GFX_FLIP_X     (1 << 0)
#define GFX_FLIP_Y     (1 << 1)
#define GFX_ROTATE_CW  (1 << 2)
#define GFX_ROTATE_CCW (1 << 3)

void main() {
    vec2 texcoord = a_texcoord0;
    const int flags = int(a_flags);

    if ((flags & GFX_FLIP_X) != 0) {
        texcoord.x = 1.0 - texcoord.x;
    }

    if ((flags & GFX_FLIP_Y) != 0) {
        texcoord.y = 1.0 - texcoord.y;
    }

    layer = int(a_layer);
    uv = a_uvmin + (texcoord * (a_uvmax - a_uvmin));
    color = a_color;
    depth = a_z;

    const vec2 ipos = a_offset + (a_scale * a_position);
    // TODO: negative for Z to work?
    gl_Position = proj * view * model * vec4(ipos, a_z, 1.0);
}
@end

@fs fs
uniform sampler2DArray atlas;

flat in int layer;
in vec2 uv;
in vec4 color;
in float depth;

out vec4 frag_color;

void main() {
    frag_color = color * texture(atlas, vec3(uv, layer));
    if (frag_color.a < 0.0001) {
        discard;
    }
}
@end

@program batch vs fs
