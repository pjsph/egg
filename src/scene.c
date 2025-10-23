#include "scene.h"
#include "asset.h"

#include "defines.h"

void scene_create(escene_desc *description, escene *scene) {
    scene->id = 16; // TODO
    // create assets
    scene->bg_asset_id = asset_register(description->bg, ASSET_TEXTURE);
}

void scene_destroy(escene *scene) {

}

void scene_load(escene *scene) {
    asset_load(scene->bg_asset_id);
}

void scene_render(escene *scene, int width, int height) {
    renderer_draw_asset(scene->bg_asset_id, 0, 0, width, height);
    if(!scene->COMP_SPRITE) {
        return;
    }
    u32 sprite_c_id[1] = { scene->COMP_SPRITE };
    u32 entities[64];
    u32 count;
    ecs_get_entities_with_components(scene, sprite_c_id, 1, entities, &count);
    for(int i = 0; i < count; ++i) {
        u32 x = 0, y = 0;
        sprite_c *sprite = (sprite_c*)ecs_get_component_of(scene, entities[i], scene->COMP_SPRITE, sizeof(sprite_c));
        if(ecs_entity_has_component(scene, entities[i], scene->COMP_POSITION)) {
            position_c *position = (position_c*)ecs_get_component_of(scene, entities[i], scene->COMP_POSITION, sizeof(position_c));
            x = position->x;
            y = position->y;
        }
        renderer_draw_asset(sprite->asset_id, x, y, 64, 64);
    }
}

void scene_add_entity(escene *scene, u32 entity) {
    if(!scene->entities[entity]) {
        ++scene->entity_count;
    }
    scene->entities[entity] = 1;
}

// -- ECS --

u32 ecs_entity_create(escene *scene, u64 components_mask) {
    u32 id = scene->curr_entity_id++;
    // TODO: add right components
    return id;
}

void ecs_entity_destroy(escene *scene, u32 entity) {
    // TODO: scene remove
}

// TODO: store max size for each pool?
// for now let's assume that each pool has 2048 slots
void ecs_component_create(escene *scene, u32 component, void *pool) {
    // TODO: check if component with same id already exists
    scene->components[component] = pool;
}

void ecs_component_destroy(escene *scene, u32 component) {
    scene->components[component] = (void*)0;
}

void ecs_entity_add_component(escene *scene, u32 entity, u32 component) {
    scene->component_masks[component] |= 1 << entity;
}

u8 ecs_entity_has_component(escene *scene, u32 entity, u32 component) {
    return (scene->component_masks[component] >> entity) & 1;
}

void *ecs_get_component_of(escene *scene, u32 entity, u32 component, u32 component_size) {
    return (char *)scene->components[component] + entity * component_size;
}

// TODO: max 64 entities here...
u64 ecs_get_entities_with_components(escene *scene, u32 *components, u32 c_length, u32 *entities, u32 *e_length) {
    u64 entities_mask = -1;
    for(int i = 0; i < c_length; ++i) {
        entities_mask &= scene->component_masks[components[i]];
    }
    u64 tmp = entities_mask;
    u32 count = 0;
    for(int i = 0; i < sizeof(u64) * 8; ++i) {
        if(tmp & 1) {
            entities[count++] = i;
        }
        tmp >>= 1;
    }
    *e_length = count;
    return entities_mask;
}
