#include "../window.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <xcb/xcb.h>
#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>

#include <xcb/xproto.h>

// NOT IMPLEMENTED/UPDATED YET

typedef struct linux_handle_info {
    xcb_connection_t *connection;
    xcb_screen_t *screen;
} linux_handle_info;

typedef struct ewindow_backend_state {
    xcb_window_t window;
} ewindow_backend_state;

typedef struct display_backend_state {
    Display *display;
    linux_handle_info handle;
    xcb_screen_t *screen;

    ewindow *current_window; // tmp
    ewindow **windows;
} display_backend_state;

u32 display_backend_size = sizeof(display_backend_state);
static display_backend_state *display_state;

u8 display_backend_init(struct display_backend_state *state) {
    if(!state)
        return false;

    display_state = state;

    display_state->display = XOpenDisplay(NULL);
    display_state->handle.connection = XGetXCBConnection(display_state->display);

    if(xcb_connection_has_error(display_state->handle.connection)) {
        return false;
    }

    const struct xcb_setup_t *setup = xcb_get_setup(display_state->handle.connection);

    xcb_screen_iterator_t it = xcb_setup_roots_iterator(setup);
    for(i32 s = 0; s < 1; ++s) {
        xcb_screen_next(&it);
    }

    display_state->screen = it.data;
    display_state->handle.screen = display_state->screen;

    return true;
}

u8 ewindow_create(ewindow_config *config, ewindow *window) {
    if(!window)
        return false;

    window->width = config->width;
    window->height = config->height;

    window->backend_state = malloc(sizeof(ewindow_backend_state));

    window->backend_state->window = xcb_generate_id(display_state->handle.connection);

    u32 event_values = XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
                       XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE |
                       XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_POINTER_MOTION |
                       XCB_EVENT_MASK_STRUCTURE_NOTIFY;

    u32 value_list[] = {display_state->handle.screen->black_pixel, event_values};

    xcb_create_window(display_state->handle.connection, XCB_COPY_FROM_PARENT, window->backend_state->window, display_state->screen->root, config->x, config->y, config->width, config->height, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, display_state->screen->root_visual, XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK, value_list);

    if(config->title) {
        window->title = config->title;
    } else {
        window->title = "Egg window";
    }

    xcb_intern_atom_cookie_t utf8_string_cookie = xcb_intern_atom(display_state->handle.connection, 0, 11, "UTF8_STRING");
    xcb_intern_atom_reply_t *utf8_string_reply = xcb_intern_atom_reply(display_state->handle.connection, utf8_string_cookie, 0);

    xcb_intern_atom_cookie_t net_wm_name_cookie = xcb_intern_atom(display_state->handle.connection, 0, 12, "_NET_WM_NAME");
    xcb_intern_atom_reply_t *net_wm_name_reply = xcb_intern_atom_reply(display_state->handle.connection, net_wm_name_cookie, 0);

    xcb_change_property(display_state->handle.connection, XCB_PROP_MODE_REPLACE, window->backend_state->window, XCB_ATOM_WM_NAME, utf8_string_reply->atom, 8, strlen(window->title), window->title);

    xcb_change_property(display_state->handle.connection, XCB_PROP_MODE_REPLACE, window->backend_state->window, net_wm_name_reply->atom, utf8_string_reply->atom, 8, strlen(window->title), window->title);

    free(utf8_string_reply);
    free(net_wm_name_reply);

    // TODO: atoms for window destroy

    xcb_map_window(display_state->handle.connection, window->backend_state->window);

    i32 stream_result = xcb_flush(display_state->handle.connection);
    if(stream_result < 0) {
        printf("ERROR: while flushing xcb stream\n");
        return false;
    }

    display_state->current_window = window;

    return true;
}

u8 ewindow_should_close() {
    return false;
}
