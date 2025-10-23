#ifndef ASSET_H
#define ASSET_H

#include "defines.h"

typedef enum easset_type {
    ASSET_INVALID = 0,
    ASSET_IMAGE   = 1,
    ASSET_TEXTURE = 2,
} easset_type;

EAPI u32 asset_register(const char *path, easset_type type);
EAPI void asset_load(u32 id);
void *asset_get_data(u32 id);
 
#endif // ASSET_H
