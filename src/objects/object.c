#include "object.h"
#include "triangle.h"
#include "cube.h"
#include <string.h>

// Global object type registry
static object_type_t object_types[OT_COUNT];

void object_init(object_t *obj, object_type_index_t type) {
    memset(obj, 0, sizeof(object_t));
    obj->type = type;
    obj->type_info = get_object_type(type);
    
    // Set default values
    glm_vec3_one(obj->scale);
    glm_mat4_identity(obj->model_matrix);
    glm_vec3_one(obj->color);
    obj->alpha = 1.0f;
    obj->metallic = 0.0f;
    obj->roughness = 0.5f;

    // Call type-specific init
    if (obj->type_info && obj->type_info->init) {
        obj->type_info->init(obj);
    }
}

void object_update(object_t *obj) {
    if (obj->type_info && obj->type_info->update) {
        obj->type_info->update(obj);
    }
    object_update_model_matrix(obj);
}

void object_render(object_t *obj) {
    if (obj->type_info && obj->type_info->render) {
        obj->type_info->render(obj);
    }
}

void object_cleanup(object_t *obj) {
    if (obj->type_info && obj->type_info->cleanup) {
        obj->type_info->cleanup(obj);
    }
}

void object_set_position(object_t *obj, vec3 position) {
    glm_vec3_copy(position, obj->position);
    object_update_model_matrix(obj);
}

void object_set_rotation(object_t *obj, vec3 rotation) {
    glm_vec3_copy(rotation, obj->rotation);
    object_update_model_matrix(obj);
}

void object_set_scale(object_t *obj, vec3 scale) {
    glm_vec3_copy(scale, obj->scale);
    object_update_model_matrix(obj);
}

void object_update_model_matrix(object_t *obj) {
    glm_mat4_identity(obj->model_matrix);
    
    // Apply transformations in order: scale -> rotate -> translate
    glm_scale(obj->model_matrix, obj->scale);
    
    // Apply rotations around each axis
    glm_rotate_x(obj->model_matrix, obj->rotation[0], obj->model_matrix);
    glm_rotate_y(obj->model_matrix, obj->rotation[1], obj->model_matrix);
    glm_rotate_z(obj->model_matrix, obj->rotation[2], obj->model_matrix);
    
    glm_translate(obj->model_matrix, obj->position);
}

void register_object_types(void) {
    // Initialize all object types to NULL
    memset(object_types, 0, sizeof(object_types));
    
    // Register triangle type
    object_types[OT_TRIANGLE] = (object_type_t){
        .name = "Triangle",
        .flags = OTF_EDITACTIVE,
        .init = triangle_init,
        .update = triangle_update,
        .render = triangle_render,
        .cleanup = triangle_cleanup
    };
    
    // Register cube type
    object_types[OT_CUBE] = (object_type_t){
        .name = "Cube",
        .flags = OTF_EDITACTIVE,
        .init = cube_init,
        .update = cube_update,
        .render = cube_render,
        .cleanup = cube_cleanup
    };
}

const object_type_t *get_object_type(object_type_index_t type) {
    if (type >= 0 && type < OT_COUNT) {
        return &object_types[type];
    }
    return NULL;
} 
