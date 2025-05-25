#include "shader.h"
#include <stdio.h>
#include <stdlib.h>

static char* read_shader_source(const char* filepath) {
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open shader file: %s\n", filepath);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);

    char* source = malloc(size + 1);
    if (!source) {
        fclose(file);
        fprintf(stderr, "Failed to allocate memory for shader: %s\n", filepath);
        return NULL;
    }

    fread(source, 1, size, file);
    source[size] = '\0';
    fclose(file);
    return source;
}

static GLuint compile_shader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, 512, NULL, log);
        fprintf(stderr, "Shader compilation failed:\n%s\n", log);
    }

    return shader;
}

GLuint shader_load_program(const char* vertex_path, const char* fragment_path) {
    char* vertex_code = read_shader_source(vertex_path);
    char* fragment_code = read_shader_source(fragment_path);
    if (!vertex_code || !fragment_code) {
        fprintf(stderr, "Failed to read shader sources.\n");
        exit(1);
    }

    GLuint vertex_shader = compile_shader(GL_VERTEX_SHADER, vertex_code);
    GLuint fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fragment_code);
    free(vertex_code);
    free(fragment_code);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(program, 512, NULL, log);
        fprintf(stderr, "Shader linking failed:\n%s\n", log);
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    return program;
}

