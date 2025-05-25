#include "cube.h"
#include "../state.h"
#include <cglm/cglm.h>
#include <string.h>

// Cube vertices (36 vertices for 6 faces, 2 triangles per face)
static const float cube_vertices[] = {
    // Front face
    -0.5f, -0.5f,  0.5f,
     0.5f, -0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f,
    -0.5f, -0.5f,  0.5f,

    // Back face
    -0.5f, -0.5f, -0.5f,
     0.5f, -0.5f, -0.5f,
     0.5f,  0.5f, -0.5f,
     0.5f,  0.5f, -0.5f,
    -0.5f,  0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f,

    // Top face
    -0.5f,  0.5f, -0.5f,
     0.5f,  0.5f, -0.5f,
     0.5f,  0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f, -0.5f,

    // Bottom face
    -0.5f, -0.5f, -0.5f,
     0.5f, -0.5f, -0.5f,
     0.5f, -0.5f,  0.5f,
     0.5f, -0.5f,  0.5f,
    -0.5f, -0.5f,  0.5f,
    -0.5f, -0.5f, -0.5f,

    // Right face
     0.5f, -0.5f, -0.5f,
     0.5f,  0.5f, -0.5f,
     0.5f,  0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,
     0.5f, -0.5f,  0.5f,
     0.5f, -0.5f, -0.5f,

    // Left face
    -0.5f, -0.5f, -0.5f,
    -0.5f,  0.5f, -0.5f,
    -0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f,
    -0.5f, -0.5f,  0.5f,
    -0.5f, -0.5f, -0.5f
};

void cube_init(object_t *obj) {
    cube_t *cube = &obj->data.cube;
    
    // Copy vertex data
    memcpy(cube->vertices, cube_vertices, sizeof(cube_vertices));
    
    // Initialize rotation and scale
    cube->rotation[0] = 0.0f;
    cube->rotation[1] = 0.0f;
    cube->rotation[2] = 0.0f;
    cube->scale = 1.0f;
    
    // Set default material properties
    glm_vec3_copy((vec3){0.0f, 0.5f, 1.0f}, obj->color);  // Blue color
    obj->alpha = 1.0f;
    obj->metallic = 0.0f;
    obj->roughness = 0.5f;
}

void cube_update(object_t *obj) {
    cube_t *cube = &obj->data.cube;
    // Do not auto-increment rotation
    // cube->rotation[0] += 0.01f;
    // cube->rotation[1] += 0.01f;
    // if (cube->rotation[0] > 360.0f) cube->rotation[0] -= 360.0f;
    // if (cube->rotation[1] > 360.0f) cube->rotation[1] -= 360.0f;

    // Update object's rotation
    vec3 rotation = {cube->rotation[0], cube->rotation[1], cube->rotation[2]};
    object_set_rotation(obj, rotation);

    // Update object's scale
    vec3 scale = {cube->scale, cube->scale, cube->scale};
    object_set_scale(obj, scale);
}

void cube_render(object_t *obj) {
    cube_t *cube = &obj->data.cube;
    
    // Update vertex buffer with current vertices
    sg_update_buffer(state->gfx.cube_vbuf, &(sg_range){
        .ptr = cube->vertices,
        .size = sizeof(cube->vertices)
    });
    
    // Apply uniforms
    struct {
        vec3 color;
        float _pad1;
        mat4 model;
        mat4 view;
        mat4 projection;
    } vs_uniforms;
    
    glm_vec3_copy(obj->color, vs_uniforms.color);
    glm_mat4_copy(obj->model_matrix, vs_uniforms.model);
    glm_mat4_copy(state->view_matrix, vs_uniforms.view);
    glm_mat4_copy(state->projection_matrix, vs_uniforms.projection);
    
    sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &(sg_range){
        .ptr = &vs_uniforms,
        .size = sizeof(vs_uniforms)
    });

    // Bind vertex buffer
    sg_bindings bindings = {
        .vertex_buffers[0] = state->gfx.cube_vbuf
    };
    sg_apply_bindings(&bindings);
    
    // Draw the cube (36 vertices)
    sg_draw(0, 36, 1);
}

void cube_cleanup(object_t *obj) {
    // Nothing to clean up for now
    (void)obj;
} 
