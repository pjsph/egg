#ifndef WINDOW_H
#define WINDOW_H

#include "defines.h"

struct display_backend_state;

typedef struct ewindow_config {
    i32 x;
    i32 y;
    u32 width;
    u32 height;
    const char *title;
    const char *icon;
} ewindow_config;

typedef struct ewindow {
    u64 id;
    const char* title;
    u16 width;
    u16 height;
    const char *icon;
    struct window_backend_state *backend_state;
} ewindow;

EAPI u8 display_backend_init(struct display_backend_state *state);

EAPI u8 ewindow_create(ewindow_config *config, u64 *window_id);
EAPI u8 ewindow_should_close(u64 window_id);
EAPI u8 ewindow_pump_all();
EAPI u8 ewindow_destroy(u64 window_id);

#endif // WINDOW_H
