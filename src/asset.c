#include "asset.h"

#include "raylib.h"

#define MAX_ASSETS 2048

typedef struct easset {
    u32 id;
    // lookup to the data -- path for now
    const char *path;
    easset_type type;
    // underlying type -- only RL Image for now
    union {
        Image image;
        Texture2D texture;
    };
} easset;

struct {
    easset assets[MAX_ASSETS];
    // TODO: custom allocator for assets
    u32 head;
} asset_manager = { .assets = {0}, .head = 0 };

u32 asset_register(const char *path, easset_type type) {
    if(asset_manager.head > MAX_ASSETS - 1) {
        // no more memory
        return false;
    }

    easset *asset = &asset_manager.assets[asset_manager.head];
    asset->id = asset_manager.head;
    asset->path = path;
    asset->type = type;
    ++asset_manager.head;
    return asset->id;
}

void asset_load(u32 id) {
    if(asset_manager.head < id) {
        // ERROR
        return;
    }

    easset *asset = &asset_manager.assets[id];
    switch(asset->type) {
        case ASSET_IMAGE:
            asset->image = LoadImage(asset->path);
            break;
        case ASSET_TEXTURE:
            asset->texture = LoadTexture(asset->path);
            break;
    }
}

void *asset_get_data(u32 id) {
    void *data = NULL;

    if(asset_manager.head < id) {
        // ERROR
        return data;
    }

    easset *asset = &asset_manager.assets[id];
    switch(asset->type) {
        case ASSET_IMAGE:
            data = &asset->image;
            break;
        case ASSET_TEXTURE:
            data = &asset->texture;
            break;
    }

    return data;
}
