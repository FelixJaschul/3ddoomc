#pragma once

#include "defs.h"

// adds vertex to level
vertex_t *vertex_new(level_t *level, vec2s pos);

// removes vertex from level
void vertex_delete(level_t *level, vertex_t *vertex);

// update vertex fields after movement
void vertex_recalculate(level_t *level, vertex_t *vertex);

// set vertex position
void vertex_set(level_t *level, vertex_t *vertex, vec2s pos);

// gets all walls attached to a vertex
int vertex_walls(
    level_t *level, vertex_t *v, wall_t **walls, int n);

// gets all sides attached to a vertex
int vertex_sides(
    level_t *level, vertex_t *v, side_t **sides, int n);

// gets other vertex on wall with this vertex
// NULL if wall does not have vertex v on it
vertex_t *vertex_other(
    level_t *level, vertex_t *v, wall_t *wall);

// finds wall two vertices share, NULL if they share no wall
wall_t *vertices_shared_wall(
    const level_t *level,
    const vertex_t *v0,
    const vertex_t *v1);

// finds sector two vertices share, NULL if they share no sector
sector_t *vertices_shared_sector(
    const level_t *level,
    const vertex_t *v0,
    const vertex_t *v1);
