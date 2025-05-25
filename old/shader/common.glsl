@block common

@include ../gfx/renderer_types.h
@include ../shared_defs.h
@include ../config.h

#define PI 3.14159265359
#define TAU (2.0 * PI)
#define PI_2 (PI / 2.0)
#define PI_4 (PI / 4.0)

// atan2(x, y) = abs(atan(abs(y / x)) + PI * min(0, sign(x))) * sign(y)
// sign(y) causes the discontinuity so we just need a smooth sign function
// for example, a smoothstep mapped from -1 to 1
float satan2(in vec2 p, in float w) { // :)
    float a = abs(p.x) < 1e-8 ? PI_2 : atan(abs(p.y / p.x));
    float sy = 2.0 * smoothstep(-w, w, p.y) - 1.0;
    return abs(a + PI * min(0.0, sign(p.x))) * sy;
}

float atan2(in float y, in float x) {
    bool s = (abs(x) > abs(y));
    return mix(PI/2.0 - atan(x,y), atan(y,x), s);
}

// https://developer.download.nvidia.com/cg/findLSB.html
int find_lsb(int x)
{
  int i;
  int mask;
  int res = -1;
  for(i = 0; i < 32; i++) {
    mask = 1 << i;
    if ((x & mask) != 0) {
      res = i;
      break;
    }
  }
  return res;
}

void lookup_data(
        sampler2DArray data_image,
        int type_index,
        int index,
        out vec4 data[RENDERER_DATA_IMG_WIDTH_VEC4S]) {
    const vec3 data_size = textureSize(data_image, 0);
    for (int i = 0; i < RENDERER_DATA_IMG_WIDTH_VEC4S; i++) {
        const vec3 coords =
            vec3(
                i / data_size.x,
                int(index) / data_size.y,
                type_index);
        data[i] = texture(data_image, coords);
    }
}

vec4 post_process(
        vec4 color,
        sampler3D palette,
        vec3 pos_w,
        vec3 pos_v,
        vec3 cam_pos,
        float light) {
    color *= vec4(vec3(light), 1.0);
    color.rgb =
        mix(
            color.rgb,
            vec3(0.1, 0.1, 0.1),
            clamp(length(-pos_v.z) / 32.0, 0, 1));

    const vec3 poffset = (1.0 / textureSize(palette, 0)) * 0.5;
    // color.rgb = texture(palette, clamp(color.rgb - poffset, vec3(0), vec3(1))).rgb;
    return color;
}
@end
