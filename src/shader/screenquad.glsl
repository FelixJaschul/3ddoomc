@module screenquad

@include ctypes.glsl

@vs vs
in vec2 position;
in vec2 texcoord0;

uniform vs_params {
	mat4 model;
    mat4 view;
    mat4 proj;
};

out vec2 uv;
void main() {
    gl_Position = proj * model * view * vec4(position, 0.0, 1.0);
    uv = texcoord0;
}
@end

@fs fs
uniform sampler2D tex;

uniform fs_params {
    int is_stencil;
};

in vec2 uv;
out vec4 frag_color;
void main() {
    if (is_stencil != 0) {
        frag_color = vec4(texture(tex, uv).aaa, 1.0);
    } else {
        frag_color = texture(tex, uv);
    }
}
@end

@program screenquad vs fs
