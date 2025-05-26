@module basic

@include ctypes.glsl

@vs vs
in vec3 position;
in vec2 texcoord0;

uniform vs_params {
	mat4 model;
    mat4 view;
    mat4 proj;
};

out vec2 uv;
void main() {
    gl_Position = proj * model * view * vec4(position, 1.0);
    uv = texcoord0;
}
@end

@fs fs
/* uniform sampler2D tex; */
in vec2 uv;
out vec4 frag_color;
void main() {
    frag_color = vec4(uv, 0.0, 1.0); //texture(tex, uv);
}
@end

@program basic vs fs
