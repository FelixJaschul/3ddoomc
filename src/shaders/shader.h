#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>

// Load and compile a vertex + fragment shader and link them into a program.
GLuint shader_load_program(const char* vertex_path, const char* fragment_path);

#endif

