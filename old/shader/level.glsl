@module level

@include ctypes.glsl
@include common.glsl

@vs vs
@include_block common

in vec3 a_position;
in vec2 a_texcoord0;
in float a_id;
in float a_index;
in float a_flags;

uniform sampler2DArray data_image;

uniform vs_params {
    mat4 view;
    mat4 proj;
};

out vec3 pos_w;
out vec3 pos_v;
out vec2 uv;
flat out int id;
flat out int index;
flat out int flags;
flat out vec4 data[RENDERER_DATA_IMG_WIDTH_VEC4S];

void main() {
    pos_w = a_position;
    pos_v = vec3(view * vec4(a_position, 1.0)).xyz;
    gl_Position = proj * vec4(pos_v, 1.0);
    uv = a_texcoord0;
    id = int(a_id);
    index = int(a_index);
    flags = int(a_flags);
    lookup_data(data_image, find_lsb(id >> 16), int(a_index), data);
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
    int is_portal_pass;
};

in vec3 pos_w;
in vec3 pos_v;
in vec2 uv;
flat in int id;
flat in int index;
flat in int flags;
flat in vec4 data[RENDERER_DATA_IMG_WIDTH_VEC4S];

layout(location=0) out vec4 frag_color;
layout(location=1) out vec4 frag_info;

void main() {
    if (is_portal_pass == 0 && (flags & RENDERER_VFLAG_PORTAL) != 0) {
        discard;
    }

    int texture_id;

    side_render_data_t rd_side;
    side_render_data_unpack(data, rd_side);

    sector_render_data_t rd_sector;
    sector_render_data_unpack(data, rd_sector);

    decal_render_data_t rd_decal;
    decal_render_data_unpack(data, rd_decal);

    if (((id >> 16) & T_SIDE) != 0) {
        vec4 data_sector[RENDERER_DATA_IMG_WIDTH_VEC4S];
        lookup_data(data_image, T_SECTOR_INDEX, int(rd_side.sector_index), data_sector);
        sector_render_data_unpack(data_sector, rd_sector);

        const int stflags = int(rd_side.stflags);
        texture_id = int(rd_side.tex_mid);

        if ((stflags & STF_EZPORT) != 0) {
            if ((flags & RENDERER_VFLAG_SEG_BOTTOM) != 0) {
                texture_id = int(rd_side.tex_low);
            } else if ((flags & RENDERER_VFLAG_SEG_TOP) != 0) {
                texture_id = int(rd_side.tex_high);
            } else {
                texture_id = int(rd_side.tex_mid);
            }
        } else {
            float z_base;
            if ((flags & RENDERER_VFLAG_SEG_BOTTOM) != 0) {
                z_base = rd_side.z_floor;
            } else if ((flags & RENDERER_VFLAG_SEG_TOP) != 0) {
                z_base = rd_side.nz_ceil;
            } else {
                z_base = rd_side.z_floor;
            }

            float split_z = pos_w.z - z_base;

            float
                split_bottom = rd_side.split_bottom,
                split_top = rd_side.split_top;

            if ((stflags & STF_BOT_ABS) != 0) {
                split_bottom -= z_base;
            }

            if ((stflags & STF_TOP_ABS) != 0) {
                split_top -= z_base;
            }

            if (split_z < split_bottom) {
                texture_id = int(rd_side.tex_low);
            } else if (split_z > split_top) {
                texture_id = int(rd_side.tex_high);
            } else {
                texture_id = int(rd_side.tex_mid);
            }
        }
    } else if (((id >> 16) & T_SECTOR) != 0) {
        texture_id =
            (flags & RENDERER_VFLAG_IS_CEIL) != 0 ?
                int(rd_sector.tex_ceil)
                : int(rd_sector.tex_floor);
    } else if (((id >> 16) & T_DECAL) != 0) {
        texture_id = int(rd_decal.tex);

        vec4 data_sector[RENDERER_DATA_IMG_WIDTH_VEC4S];
        lookup_data(data_image, T_SECTOR_INDEX, int(rd_decal.sector_index), data_sector);
        sector_render_data_unpack(data_sector, rd_sector);

        const int side_index = int(rd_decal.side_index);

        if (side_index != 0) {
            vec4 data_side[RENDERER_DATA_IMG_WIDTH_VEC4S];
            lookup_data(data_image, T_SIDE_INDEX, side_index, data_side);
            side_render_data_unpack(data_side, rd_side);
        }
    }

    const int atlas_n = textureSize(atlas_coords, 0).x;
    const vec4 atlas_coord = texture(atlas_coords, vec2(texture_id / float(atlas_n), 0));
    const vec2
        atlas_min = atlas_coord.xy,
        atlas_max = atlas_coord.zw,
        atlas_size = atlas_max - atlas_min;
    const int atlas_layer =
        int(texture(atlas_layers, vec2(texture_id / float(atlas_n), 0)).r);

    const vec2 unit = vec2(1.0) / textureSize(atlas, 0).xy;

    float light;
    vec2 uv1;
    vec2 offs;
    if (((id >> 16) & T_SIDE) != 0) {
        offs = vec2(uv.s * rd_side.len, pos_w.z);
        offs -= (rd_side.offsets / PX_PER_UNIT);

        //float angle = atan2(rd_side.normal.x, rd_side.normal.y);
        //if (angle < PI) { angle = angle + TAU; }
        //else if (angle > PI) { angle = angle - TAU; }
        light = rd_sector.light;
        light += 0.1f * (sin(satan2(rd_side.normal.yx, 0.01f)));

        side_frag_data_t fd;
        fd.pos = vec2((1.0 - uv.s) * rd_side.len, pos_w.z);
        fd.id = id;
        frag_info = side_frag_data_pack(fd);

        uv1 = atlas_min + mod((offs * PX_PER_UNIT) * unit, atlas_size);
    } else if (((id >> 16) & T_SECTOR) != 0) {
        offs = pos_w.xy;
        light = rd_sector.light;

        sector_frag_data_t fd;
        fd.pos = offs;
        fd.is_ceil = (flags & RENDERER_VFLAG_IS_CEIL) != 0 ? 1 : 0;
        fd.id = id;
        frag_info = sector_frag_data_pack(fd);

        uv1 = atlas_min + mod((offs * PX_PER_UNIT) * unit, atlas_size);
    } else if (((id >> 16) & T_DECAL) != 0) {
        light = rd_sector.light;

        decal_frag_data_t fd;
        fd.id = id;
        if (int(rd_decal.side_index) != 0) {
            fd.pos = vec2(length(pos_w.xy - rd_side.origin), pos_w.z);
        } else {
            fd.pos = vec2(pos_w.xy);
            fd.is_ceil = int(rd_decal.plane_type) == PLANE_TYPE_CEIL ? 1 : 0;
        }
        frag_info = decal_frag_data_pack(fd);
        uv1 = atlas_min + uv * atlas_size;
    }

    frag_color =
        post_process(
            texture(atlas, vec3(uv1, atlas_layer)),
            palette,
            pos_w,
            pos_v,
            cam_pos,
            light);
    frag_info.a = id;
}
@end

@program level vs fs
