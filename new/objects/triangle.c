#include "triangle.h"
#include "../state.h"
#include <cglm/cglm.h>
#include <string.h>

void triangle_init(object_t *obj) {
	triangle_t *triangle = &obj->data.triangle;
	
	// Initialize vertices for an equilateral triangle
	float vertices[] = {
		-0.5f, -0.5f, 0.0f,  // bottom left
		 0.5f, -0.5f, 0.0f,  // bottom right
		 0.0f,  0.5f, 0.0f   // top
	};
	
	memcpy(triangle->vertices, vertices, sizeof(vertices));
	triangle->rotation = 0.0f;
	triangle->scale = 1.0f;
	
	// Set default material properties
	glm_vec3_copy((vec3){1.0f, 0.0f, 0.0f}, obj->color);  // Red color
	obj->alpha = 1.0f;
	obj->metallic = 0.0f;
	obj->roughness = 0.5f;
}

void triangle_update(object_t *obj) {
	triangle_t *triangle = &obj->data.triangle;
	// Do not auto-increment rotation
	// triangle->rotation += 0.01f;
	// if (triangle->rotation > 360.0f) {
	//     triangle->rotation -= 360.0f;
	// }

	// Update object's rotation
	vec3 rotation = {0.0f, 0.0f, triangle->rotation};
	object_set_rotation(obj, rotation);
	
	// Update object's scale
	vec3 scale = {triangle->scale, triangle->scale, triangle->scale};
	object_set_scale(obj, scale);
}

void triangle_render(object_t *obj) {
	triangle_t *triangle = &obj->data.triangle;
	
	// Update vertex buffer with current vertices
	sg_update_buffer(state->gfx.triangle_vbuf, &(sg_range){
		.ptr = triangle->vertices,
		.size = sizeof(triangle->vertices)
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
		.vertex_buffers[0] = state->gfx.triangle_vbuf
	};
	sg_apply_bindings(&bindings);
	
	// Draw the triangle
	sg_draw(0, 3, 1);
}

void triangle_cleanup(object_t *obj) {
	// Nothing to clean up for now
	(void)obj;
} 
