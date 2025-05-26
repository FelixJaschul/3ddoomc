@module sprite

@include ctypes.glsl
@include common.glsl

@vs vs
@include_block common

in vec3 a_position;
in vec2 a_texcoord0;

// per-instance
in vec3 a_offset;
in vec2 a_size;
in vec4 a_color;
in float a_id;
in float a_index;
in float a_flags;

uniform sampler2DArray data_image;

uniform vs_params {
    float yaw;
    mat4 view;
    mat4 proj;
};

out vec3 pos_w;
out vec3 pos_v;
out vec2 uv;
out vec4 color;
flat out int id;
flat out vec4 data[RENDERER_DATA_IMG_WIDTH_VEC4S];

void main() {
    const float a = -yaw - PI_2;
    pos_w = vec3(
        a_offset.x + a_size.x * ((a_position.x - 0.5f) * cos(a)),
        a_offset.y + a_size.x * ((a_position.x - 0.5f) * sin(a)),
        a_offset.z + (a_size.y * a_position.z));
    pos_v = (view * vec4(pos_w, 1.0)).xyz;
    gl_Position = proj * vec4(pos_v, 1.0);
    uv = a_texcoord0;
    color = a_color + vec4(a_flags); // TODO
    id = int(a_id);

    // NOTE: ALWAYS looking up in object, regardless of type (!)
    lookup_data(data_image, T_OBJECT_INDEX, int(a_index), data);
}
@end

@fs fs
@include_block common

uniform sampler2DArray atlas;
uniform sampler2D atlas_coords;
uniform sampler2D atlas_layers;
uniform sampler3D palette;
uniform sampler2DArray data_image;

uniform fs_params {
    vec3 cam_pos;
};

in vec3 pos_w;
in vec3 pos_v;
in vec2 uv;
in vec4 color;
flat in int id;
flat in vec4 data[RENDERER_DATA_IMG_WIDTH_VEC4S];

layout(location=0) out vec4 frag_color;
layout(location=1) out vec4 frag_info;

void main() {
    sprite_render_data_t rd;
    sprite_render_data_unpack(data, rd);

    // TODO: don't lookup whole sector just for light
    vec4 data_sector[RENDERER_DATA_IMG_WIDTH_VEC4S];
    lookup_data(data_image, T_SECTOR_INDEX, int(rd.sector_index), data_sector);

    sector_render_data_t rd_sector;
    sector_render_data_unpack(data_sector, rd_sector);

    const int texture_id = int(rd.tex);

    const int atlas_n = textureSize(atlas_coords, 0).x;
    const vec4 atlas_coord = texture(atlas_coords, vec2(texture_id / float(atlas_n), 0));
    const vec2
        atlas_min = atlas_coord.xy,
        atlas_max = atlas_coord.zw,
        atlas_size = atlas_max - atlas_min;
    const int atlas_layer =
        int(texture(atlas_layers, vec2(texture_id / float(atlas_n), 0)).r);

    const vec2 unit = vec2(1.0) / textureSize(atlas, 0).xy;

    const vec2 uv1 = atlas_min + (uv * atlas_size);
    frag_color =
        post_process(
            color * texture(atlas, vec3(uv1, atlas_layer)),
            palette,
            pos_w,
            pos_v,
            cam_pos,
            rd_sector.light);

    if (frag_color.a < 0.0001) {
        discard;
    }

    frag_info.a = id;
}
@end

@program sprite vs fs
