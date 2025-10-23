#include "window.h"

#include "raylib.h"

u8 ewindow_create(ewindow *window) {
    Image icon = LoadImage(window->icon);
    InitWindow(window->width, window->height, window->title);
    SetWindowIcon(icon);
    UnloadImage(icon);
    return true;
}

u8 ewindow_should_close() {
    return WindowShouldClose();
}
