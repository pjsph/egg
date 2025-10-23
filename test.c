#include "src/engine.h"
#include "src/window.h"
#include "src/app.h"
#include "src/defines.h"
#include "src/renderer.h"
#include "src/scene.h"
#include "src/asset.h"

#include <stdio.h>

// TODOS: collisions, input, sound, networking

// TODO: put that in app or smth
escene current_scene;

struct velocity_c {
    int vx;
    int vy;
};

struct velocity_c *velocities;

sprite_c *sprites;
position_c *positions;

// TODO: replace that by and id returned by the engine (like entity)?
enum COMPONENTS {
    COMP_POSITION = 0,
    COMP_VELOCITY = 1,
    COMP_SPRITE = 2
};

u8 update(eapp *app) {
    return false;
}

u8 render(eapp *app) {
    renderer_clear();
    if(current_scene.id) {
        scene_render(&current_scene, app->window.width, app->window.height); // TODO: size infos in scene?
    }
    return true;
}

// we can define variables that have a static lifetime here
#define INIT()                    \
    struct position_c pos[2048];  \
    positions = pos;              \
    struct velocity_c vel[2048];  \
    velocities = vel;             \
    sprite_c spr[2048];           \
    sprites = spr


u8 init(eapp *app) {
    escene_desc scene_desc = { .bg = "assets/bg.png" };
    escene scene = {0};
    scene_create(&scene_desc, &scene);
    printf("scene %d created, ready to be loaded\n", scene.id);
    scene_load(&current_scene);

    ecs_component_create(&scene, COMP_SPRITE, sprites);
    ecs_component_create(&scene, COMP_POSITION, positions);
    scene.COMP_SPRITE = COMP_SPRITE;
    scene.COMP_POSITION = COMP_POSITION;

    u32 id = ecs_entity_create(&scene, 0x1);
    printf("created entity with id: %u\n", id);
    id = ecs_entity_create(&scene, 0x1);
    printf("created entity with id: %u\n", id);

    ecs_component_create(&scene, COMP_VELOCITY, velocities);

    ecs_entity_add_component(&scene, 0, COMP_POSITION);
    ecs_entity_add_component(&scene, 0, COMP_SPRITE);

    position_c *position = (position_c*)ecs_get_component_of(&scene, 0, COMP_POSITION, sizeof(position_c));
    position->x = 100;
    position->y = 275;

    sprite_c *sprite = (sprite_c*)ecs_get_component_of(&scene, 0, COMP_SPRITE, sizeof(sprite_c));
    sprite->asset_id = asset_register("assets/icon.png", ASSET_TEXTURE);
    asset_load(sprite->asset_id);

    ecs_entity_add_component(&scene, 1, COMP_POSITION);
    ecs_entity_add_component(&scene, 1, COMP_VELOCITY);

    current_scene = scene;
}

void app_new(eapp *app) {
    ewindow w = { .title = "Egg", .width = 600, .height = 400, .icon = "assets/icon.png"};
    app->window = w;

    app->init = init;
    app->update = update;
    app->render = render;
}

#include "src/entry.h"
