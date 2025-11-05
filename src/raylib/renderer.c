#include "renderer.h"
#include "asset.h"

#include "raylib.h"

u8 renderer_clear() {
    BeginDrawing();
    ClearBackground(RAYWHITE);
    EndDrawing();
    return true;
}

u8 renderer_draw_asset(u32 id, int x, int y, int width, int height) {
    Texture2D *texture = asset_get_data(id);
    DrawTexturePro(*texture, (Rectangle){ .x = 0.0f, .y = 0.0f, .width = texture->width, .height = texture->height }, (Rectangle){ .x = x, .y = y, .width = width, .height = height }, (Vector2){ .x = 0.0f, .y = 0.0f }, 0.0f, WHITE);
    /*DrawTexture(*texture, x, y, WHITE);*/
    return true;
}
