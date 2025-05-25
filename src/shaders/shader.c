#include "shader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* shader_load_file(const char* path) {
    FILE* file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open shader file: %s\n", path);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = malloc(size + 1);
    if (!buffer) {
        fprintf(stderr, "Failed to allocate memory for shader\n");
        fclose(file);
        return NULL;
    }

    size_t read_size = fread(buffer, 1, size, file);
    if (read_size != size) {
        fprintf(stderr, "Failed to read shader file: %s\n", path);
        free(buffer);
        fclose(file);
        return NULL;
    }

    buffer[size] = '\0';
    fclose(file);
    return buffer;
}

// Remove all OpenGL code below this line

