#ifndef SCENE_H
#define SCENE_H

#include "defines.h"

typedef struct sprite_c {
    u32 asset_id;
} sprite_c;

typedef struct position_c {
    int x;
    int y;
} position_c;

typedef struct collision_box {
    int x1;
    int x2;
    int y1;
    int y2;
} collision_box;

typedef struct escene_desc {
    const char *bg;
} escene_desc;

typedef struct escene {
    u32 id;
    // TODO: bg? fg?
    u32 bg_asset_id;

    // collision boxes for entities and terrain
    collision_box collision_boxes[2048];

    u32 entity_count;
    u32 entities[2048];

    // TODO: ecs in its own file
    // these are user constructed and managed
    u32 curr_entity_id;
    void *components[100];
    u64 component_masks[100];

    // these should be fully engine managed
    u32 COMP_SPRITE;
    u32 COMP_POSITION;
} escene;

EAPI void scene_create(escene_desc *description, escene *s);
EAPI void scene_destroy(escene *s);

EAPI void scene_load(escene *s);
EAPI void scene_render(escene *s, int width, int height);

EAPI void scene_add_entity(escene *scene, u32 entity);

// TODO: update, render

// -- ECS --

EAPI u32 ecs_entity_create(escene *scene, u64 components_mask);
EAPI void ecs_entity_destroy(escene *scene, u32 entity);

EAPI void ecs_component_create(escene *scene, u32 component, void *pool);
EAPI void ecs_component_destroy(escene *scene, u32 component);

EAPI void ecs_entity_add_component(escene *scene, u32 entity, u32 component);
EAPI u8 ecs_entity_has_component(escene *scene, u32 entity, u32 component);
EAPI void *ecs_get_component_of(escene *scene, u32 entity, u32 component, u32 component_size);
EAPI u64 ecs_get_entities_with_components(escene *scene, u32 *components, u32 c_length, u32 *entities, u32 *e_length);

#endif // SCENE_H
