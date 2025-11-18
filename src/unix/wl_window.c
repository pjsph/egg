// @ref: https://gaultier.github.io/blog/wayland_from_scratch.html#wayland-basics

#include "../window.h"
#include "../logger.h"
#include "../assert.h"
#include "../darray.h"
#include "../memory.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <unistd.h>

#define roundup4(n) ((n + 3) & -4)

// consts

static const u32 wayland_display_object_id = 1;
static const u16 wayland_wl_display_get_registry_opcode = 1;
static const u16 wayland_wl_registry_event_global = 0;
static const u16 wayland_wl_registry_bind_opcode = 0;
static const u16 wayland_wl_shm_create_pool_opcode = 0;
static const u16 wayland_shm_pool_event_format = 0;
static const u16 wayland_wl_display_error_event = 0;
static const u16 wayland_wl_compositor_create_surface_opcode = 0; 
static const u16 wayland_xdg_wm_base_get_xdg_surface_opcode = 2;
static const u16 wayland_xdg_surface_get_toplevel_opcode = 1;
static const u16 wayland_wl_surface_commit_opcode = 6;
static const u16 wayland_wl_surface_attach_opcode = 1;
static const u16 wayland_xdg_wm_base_event_ping = 0;
static const u16 wayland_xdg_wm_base_pong_opcode = 3;
static const u16 wayland_xdg_toplevel_event_configure = 0;
static const u16 wayland_xdg_toplevel_event_wm_capabilities = 3;
static const u16 wayland_xdg_surface_event_configure = 0;
static const u16 wayland_xdg_surface_ack_configure_opcode = 4;
static const u16 wayland_wl_shm_pool_create_buffer_opcode = 0;
static const u16 wayland_wl_surface_event_preferred_buffer_scale = 2;
static const u16 wayland_wl_surface_event_preferred_buffer_transform = 3;
static const u16 wayland_wl_buffer_event_release = 0;
static const u16 wayland_xdg_toplevel_event_close = 1;
static const u16 wayland_wl_output_event_geometry = 0;
static const u16 wayland_wl_output_event_mode = 1;
static const u16 wayland_wl_output_event_done = 2;
static const u16 wayland_wl_output_event_scale = 3;
static const u16 wayland_wl_output_event_name = 4;
static const u16 wayland_wl_output_event_description = 5;
static const u16 wayland_wl_surface_event_enter = 0;
static const u16 wayland_wl_buffer_destroy_opcode = 0;
static const u16 wayland_wl_shm_pool_destroy_opcode = 1;
static const u16 wayland_wl_display_event_delete_id = 1;
static const u16 wayland_xdg_toplevel_destroy_opcode = 0;
static const u16 wayland_xdg_surface_destroy_opcode = 0;
static const u16 wayland_wl_surface_destroy_opcode = 0;
static const u16 wayland_wl_seat_get_keyboard_opcode = 1;
static const u16 wayland_wl_seat_event_name = 1;
static const u16 wayland_wl_seat_event_capabilities = 0;
static const u16 wayland_wl_keyboard_event_keymap = 0;
static const u16 wayland_wl_keyboard_event_enter = 1;
static const u16 wayland_wl_keyboard_event_leave = 2;
static const u16 wayland_wl_keyboard_event_key = 3;
static const u16 wayland_wl_keyboard_event_modifiers = 4;
static const u16 wayland_wl_keyboard_event_repeat_info = 5;
static const u32 wayland_format_xrgb8888 = 1;
static const u16 wayland_header_size = 8;
static const u32 color_channels = 4;

typedef enum display_state_t {
    DISPLAY_STATE_NONE,
    DISPLAY_STATE_INIT,
} display_state_t;

typedef enum window_state_t {
    WINDOW_STATE_NONE,
    WINDOW_STATE_SURFACE_ACKED_CONFIGURE,
    WINDOW_STATE_SURFACE_ATTACHED,
    WINDOW_STATE_SHOULD_CLOSE,
} window_state_t;

typedef struct memchunk {
    // when delete_id event is received with this object id, the memory will be freed
    u32 wl_object; 
    u32 wl_buffers[2];
    i32 fd;
    void *data;
    u32 size;
} memchunk;

typedef struct da_windows {
    ewindow *items;
    u32 count;
    u32 capacity;
} da_windows;

typedef struct window_backend_state {
    u32 wl_surface;
    u32 xdg_surface;
    u32 xdg_toplevel;
    u32 wl_shm_pool;
    u32 wl_buffer;

    u32 shm_pool_size;
    u8 *shm_pool_data;
    i32 shm_fd;

    u32 width;
    u32 height;

    // new state -- used on configure events
    u32 width_req;
    u32 height_req;

    window_state_t state;
} window_backend_state;

typedef struct display_backend_state {
    i32 fd;
    u32 current_id;

    u32 wl_registry;
    u32 wl_shm;
    u32 xdg_wm_base;
    u32 wl_compositor;
    u32 wl_output;
    u32 wl_seat;
    u32 wl_keyboard;

    da_windows windows;

    // old memory chunks to be freed on delete_id events
    memchunk old_memchunks[255];

    display_state_t state;
} display_backend_state;

// u32 display_backend_size = sizeof(display_backend_state);
static display_backend_state display_state;

// RPCs

static u8 wayland_wl_registry_bind(u32 name, char *interface, u32 interface_len, u32 version, u32 *state_interface);
static u8 wayland_wl_seat_get_keyboard();
static u8 wayland_wl_shm_create_pool(window_backend_state *backend_state);
static u8 wayland_wl_shm_pool_create_buffer(window_backend_state *backend_state);
static u8 wayland_wl_compositor_create_surface(window_backend_state *backend_state);
static u8 wayland_xdg_wm_base_get_xdg_surface(window_backend_state *backend_state);
static u8 wayland_xdg_surface_get_toplevel(window_backend_state *backend_state);
static u8 wayland_wl_surface_commit(window_backend_state *backend_state);
static u8 wayland_wl_surface_attach(window_backend_state *backend_state);
static u8 wayland_xdg_wm_base_pong(u32 ping);
static u8 wayland_xdg_surface_ack_configure(window_backend_state *backend_state, u32 serial);
static u8 wayland_wl_buffer_destroy(window_backend_state *backend_state);
static u8 wayland_wl_shm_pool_destroy(window_backend_state *backend_state);
static u8 wayland_xdg_toplevel_destroy(window_backend_state *backend_state);
static u8 wayland_xdg_surface_destroy(window_backend_state *backend_state);
static u8 wayland_wl_surface_destroy(window_backend_state *backend_state);

//

static u8 ewindow_pump(ewindow *window, u32 object_id, u16 opcode, u8 **msg, u64 *msg_len);
static ewindow *get_window(u64 window_id);
static u8 window_create_surface(window_backend_state *backend_state);
static u8 window_destroy_surface(window_backend_state *backend_state);
static u8 window_alloc_memory(u32 size, void **shm_pool_data, i32 *shm_fd);
static u8 window_unalloc_memory(u32 size, void *shm_pool_data, i32 shm_fd);
static u8 window_unbind_memory(window_backend_state *backend_state);
static u8 window_render(window_backend_state *backend_state);

static void write_u32(u8 *buf, u64 *buf_idx, u32 val);
static void write_u16(u8 *buf, u64 *buf_idx, u16 val);

//

u8 display_backend_init(struct display_backend_state *state) {
    // TODO: see if this works better than malloc
    display_state = (display_backend_state) {0};
    state = &display_state;

    if(!state) {
        return false;
    }

    // open connection to the compositor

    char *xdg_runtime_dir = getenv("XDG_RUNTIME_DIR");
    if(xdg_runtime_dir == NULL) {
        EERROR("$XDG_RUNTIME_DIR not set");
        return false;
    }

    u64 xdg_runtime_dir_len = strlen(xdg_runtime_dir);

    struct sockaddr_un addr = { .sun_family = AF_UNIX };
    EASSERT(xdg_runtime_dir_len <= sizeof(addr.sun_path) - 1);

    u32 socket_path_len = 0;
    memcpy(addr.sun_path, xdg_runtime_dir, xdg_runtime_dir_len);
    socket_path_len += xdg_runtime_dir_len;

    addr.sun_path[socket_path_len++] = '/';

    char *wayland_display = getenv("WAYLAND_DISPLAY");
    if(wayland_display == NULL) {
        char wayland_display_default[] = "wayland-0";
        memcpy(addr.sun_path + socket_path_len, wayland_display_default, 9);
        socket_path_len += 9;
    } else {
        u64 wayland_display_len = strlen(wayland_display);
        memcpy(addr.sun_path + socket_path_len, wayland_display, wayland_display_len);
        socket_path_len += wayland_display_len;
    }

    i32 fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(fd == -1) {
        EERROR("unable to create AF_UNIX socket");
        return false;
    }
    if(connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        // TODO: close socket on error
        EERROR("unable to connect to Wayland socket %s", addr.sun_path);
        return false;
    }

    state->fd = fd;
    state->current_id = wayland_display_object_id;

    // create the registry

    u8 msg[128] = "";
    u64 msg_idx = 0;

    write_u32(msg, &msg_idx, wayland_display_object_id);

    write_u16(msg, &msg_idx, wayland_wl_display_get_registry_opcode);

    u16 final_msg_size = wayland_header_size + sizeof(u32);
    write_u16(msg, &msg_idx, final_msg_size);

    write_u32(msg, &msg_idx, ++state->current_id);

    if(msg_idx != send(state->fd, msg, final_msg_size, MSG_DONTWAIT)) {
        // TODO: close socket on error
        EERROR("failed to create Wayland registry");
        return false;
    }

    state->wl_registry = state->current_id;

    EDEBUG("created display backend: -> wl_display@%u.get_registry: wl_registry=%u", wayland_display_object_id, state->current_id);

    // pump until all interfaces are bound
    while(state->wl_shm == 0 || state->xdg_wm_base == 0 || state->wl_compositor == 0) {
        ewindow_pump_all();
    }

    darray_reserve(&display_state.windows, 2);

    state->state = DISPLAY_STATE_INIT;

    return true;
}

u8 ewindow_create(ewindow_config *config, u64 *window_id) {
    EASSERT(window_id != 0);
    EASSERT(display_state.state == DISPLAY_STATE_INIT);

    darray_append(&display_state.windows, (ewindow) { .id = (f64)rand() / RAND_MAX * ULLONG_MAX });

    ewindow *window = &darray_last(&display_state.windows);
    *window_id = window->id;

    window->backend_state = ealloc(sizeof(window_backend_state));
    memset(window->backend_state, 0, sizeof(window_backend_state));

    window_backend_state *backend_state = window->backend_state;

    window->width = config->width;
    window->height = config->height;
    backend_state->width = window->width;
    backend_state->height = window->height;

    backend_state->shm_pool_size = window->height * window->height * color_channels;

    // first initial allocation
    window_alloc_memory(backend_state->shm_pool_size, (void *)&backend_state->shm_pool_data, &backend_state->shm_fd);

    window_create_surface(backend_state); 

    // ewindow_pump_all();

    EDEBUG("created new window");

    return true; 
}

u8 ewindow_destroy(u64 window_id) {
    ewindow *window = get_window(window_id);
    EASSERT(window != NULL);

    window_backend_state *backend_state = window->backend_state;

    window_unbind_memory(backend_state);
    window_destroy_surface(backend_state);

    efree(backend_state);

    for(u32 i = 0; i < display_state.windows.count; ++i) {
        if(&display_state.windows.items[i] == window) {
            darray_remove(&display_state.windows, i);
            break;
        }
    }

    EDEBUG("destroyed window");

    return true;
}

u8 ewindow_pump_all() {
    u8 read_buf[4096] = "";
    u8 cmsg_buf[4096] = "";
    struct iovec iov = { .iov_base = read_buf, .iov_len = sizeof(read_buf) };
    struct msghdr msghdr = {
        .msg_iov = &iov,
        .msg_iovlen = 1,
        .msg_control = cmsg_buf,
        .msg_controllen = sizeof(cmsg_buf),
    };

    // use recvmsg as some events contain ancillary data
    i64 read_bytes = recvmsg(display_state.fd, &msghdr, 0);
    // i64 read_bytes = recv(display_state.fd, read_buf, sizeof(read_buf), 0);

    if(read_bytes == -1) {
        EERROR("failed to read window messages. Is the display system still alive?");
        return false;
    }

    u8 *msg = read_buf;
    u64 msg_len = read_bytes;

    EDEBUG("received data: %u bytes", msg_len);

    while(msg_len > 0) {
        EASSERT(msg_len >= 8); 
        u32 object_id = *(u32 *)msg;
        msg += sizeof(u32); msg_len -= sizeof(u32);
        EASSERT_MSG(object_id <= display_state.current_id, "Received object_id = %u when current_id = %u", object_id, display_state.current_id);
        u16 opcode = *(u16 *)msg;
        msg += sizeof(u16); msg_len -= sizeof(u16);
        u16 size = *(u16 *)msg;
        msg += sizeof(u16); msg_len -= sizeof(u16);

        // global events -- treated by display_state

        if(object_id == display_state.wl_registry && opcode == wayland_wl_registry_event_global) {
            u32 name = *(u32 *)msg;
            msg += sizeof(u32); msg_len -= sizeof(u32);
            u32 interface_len = *(u32 *)msg;
            msg += sizeof(u32); msg_len -= sizeof(u32);
            u32 padded_interface_len = roundup4(interface_len);
            char interface[512] = "";
            EASSERT(padded_interface_len <= sizeof(interface) - 1);
            memcpy(interface, msg, padded_interface_len);
            msg += padded_interface_len; msg_len -= padded_interface_len;
            EASSERT(interface[interface_len - 1] == 0);
            u32 version = *(u32 *)msg;
            msg += sizeof(u32); msg_len -= sizeof(u32);

            EDEBUG("<- wl_registry@%u.global: name=%u interface=%.*s version=%u", display_state.wl_registry, name, interface_len, interface, version);

            EASSERT(size == sizeof(object_id) + sizeof(opcode) + sizeof(size) + sizeof(name) + sizeof(interface_len) + padded_interface_len + sizeof(version));

            char wl_shm_interface[] = "wl_shm";
            if(strcmp(wl_shm_interface, interface) == 0) {
                wayland_wl_registry_bind(name, interface, interface_len, version, &display_state.wl_shm);
            }

            char xdg_wm_base_interface[] = "xdg_wm_base";
            if(strcmp(xdg_wm_base_interface, interface) == 0) {
                wayland_wl_registry_bind(name, interface, interface_len, version, &display_state.xdg_wm_base);
            }

            char wl_compositor_interface[] = "wl_compositor";
            if(strcmp(wl_compositor_interface, interface) == 0) {
                wayland_wl_registry_bind(name, interface, interface_len, version, &display_state.wl_compositor);
            }

            char wl_output_interface[] = "wl_output";
            if(strcmp(wl_output_interface, interface) == 0) {
                wayland_wl_registry_bind(name, interface, interface_len, version, &display_state.wl_output);
            }

            char wl_seat_interface[] = "wl_seat";
            if(strcmp(wl_seat_interface, interface) == 0) {
                wayland_wl_registry_bind(name, interface, interface_len, version, &display_state.wl_seat);
                wayland_wl_seat_get_keyboard();
            }

            continue;
        }

        if(object_id == display_state.wl_seat && opcode == wayland_wl_seat_event_name) {
            u32 size = *(u32 *)msg;
            msg += sizeof(u32); msg_len -= sizeof(u32);
            u32 padded_size = roundup4(size);
            char name[255] = "";
            memcpy(name, msg, padded_size);
            msg += padded_size; msg_len -= padded_size;
            EDEBUG("<- wl_seat@%u.name: name=%.*s", display_state.wl_seat, size, name);
            continue;
        }

        if(object_id == display_state.wl_seat && opcode == wayland_wl_seat_event_capabilities) {
            u32 capabilities = *(u32 *)msg;
            msg += sizeof(u32); msg_len -= sizeof(u32);
            EDEBUG("<- wl_seat@%u.capabilities: capabilities=%u", display_state.wl_seat, capabilities);
            continue;
        }

        if(object_id == display_state.wl_keyboard && opcode == wayland_wl_keyboard_event_keymap) {
            u32 format = *(u32 *)msg;
            msg += sizeof(u32); msg_len -= sizeof(u32);
            u32 size = *(u32 *)msg;
            msg += sizeof(u32); msg_len -= sizeof(u32);
            
            // retrieve fd as ancillary data
            struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msghdr);
            EASSERT(cmsg->cmsg_level == SOL_SOCKET);
            EASSERT(cmsg->cmsg_type == SCM_RIGHTS);
            i32 fd = *(i32 *)CMSG_DATA(cmsg);

            EDEBUG("<- wl_keyboard@%u.keymap: format=%u fd=%u size=%u", display_state.wl_keyboard, format, fd, size);
            continue;
        }

        if(object_id == display_state.wl_keyboard && opcode == wayland_wl_keyboard_event_repeat_info) {
            u32 rate = *(u32 *)msg;
            msg += sizeof(u32); msg_len -= sizeof(u32);
            u32 delay = *(u32 *)msg;
            msg += sizeof(u32); msg_len -= sizeof(u32);
            EDEBUG("<- wl_keyboard@%u.repeat_info: rate=%u delay=%u", display_state.wl_keyboard, rate, delay);
            continue;
        }

        if(object_id == display_state.wl_keyboard && opcode == wayland_wl_keyboard_event_enter) {
            u32 serial = *(u32 *)msg;
            msg += sizeof(u32); msg_len -= sizeof(u32);
            u32 surface = *(u32 *)msg;
            msg += sizeof(u32); msg_len -= sizeof(u32);
            u32 len = *(u32 *)msg;
            msg += sizeof(u32); msg_len -= sizeof(u32);
            char buf[256] = "";
            EASSERT(len <= sizeof(buf));
            memcpy(buf, msg, len);
            msg += len; msg_len -= len;
            EDEBUG("<- wl_keyboard@%u.enter: serial=%u surface=%u keys[%u]", display_state.wl_keyboard, serial, surface, len);
            continue;
        }

        if(object_id == display_state.wl_keyboard && opcode == wayland_wl_keyboard_event_modifiers) {
            u32 serial = *(u32 *)msg;
            msg += sizeof(u32); msg_len -= sizeof(u32);
            u32 mods_depressed = *(u32 *)msg;
            msg += sizeof(u32); msg_len -= sizeof(u32);
            u32 mods_latched = *(u32 *)msg;
            msg += sizeof(u32); msg_len -= sizeof(u32);
            u32 mods_locked = *(u32 *)msg;
            msg += sizeof(u32); msg_len -= sizeof(u32);
            u32 group = *(u32 *)msg;
            msg += sizeof(u32); msg_len -= sizeof(u32);
            EDEBUG("<- wl_keyboard@%u.modifiers: serial=%u depressed=%u latched=%u locked=%u group=%u", display_state.wl_keyboard, serial, mods_depressed, mods_latched, mods_locked, group);
            continue;
        }

        if(object_id == display_state.wl_keyboard && opcode == wayland_wl_keyboard_event_leave) {
            u32 serial = *(u32 *)msg;
            msg += sizeof(u32); msg_len -= sizeof(u32);
            u32 surface = *(u32 *)msg;
            msg += sizeof(u32); msg_len -= sizeof(u32);
            EDEBUG("<- wl_keyboard@%u.leave: serial=%u surface=%u", display_state.wl_keyboard, serial, surface);
            continue;
        }

        if(object_id == display_state.wl_keyboard && opcode == wayland_wl_keyboard_event_key) {
            u32 serial = *(u32 *)msg;
            msg += sizeof(u32); msg_len -= sizeof(u32);
            u32 time = *(u32 *)msg;
            msg += sizeof(u32); msg_len -= sizeof(u32);
            u32 key = *(u32 *)msg;
            msg += sizeof(u32); msg_len -= sizeof(u32);
            u32 state = *(u32 *)msg;
            msg += sizeof(u32); msg_len -= sizeof(u32);

            // TODO: tmp
            if(state == 1 && key == 23) {
                ewindow_config config = { .x = 0, .y = 0, .width = 600, .height = 400 };
                u64 id;
                ewindow_create(&config, &id);
            }

            EDEBUG("<- wl_keyboard@%u.key: serial=%u time=%u key=%u state=%u", display_state.wl_keyboard, serial, time, key, state);
            continue;
        }

        if(object_id == display_state.wl_shm && opcode == wayland_shm_pool_event_format) {
            u32 format = *(u32 *)msg;
            msg += sizeof(u32); msg_len -= sizeof(u32);
            EDEBUG("<- wl_shm: format=%#x", format);
            continue;
        }

        if(object_id == display_state.xdg_wm_base && opcode == wayland_xdg_wm_base_event_ping) {
            u32 ping = *(u32 *)msg;
            msg += sizeof(u32); msg_len -= sizeof(u32);
            EDEBUG("<- xdg_wm_base@%u.ping: ping=%u", ping);
            wayland_xdg_wm_base_pong(ping);
            continue;
        }

        if(object_id == display_state.wl_output && opcode == wayland_wl_output_event_name) {
            u32 size = *(u32 *)msg;
            msg += sizeof(u32); msg_len -= sizeof(u32);
            u32 padded_size = roundup4(size);
            char name[255] = "";
            memcpy(name, msg, padded_size);
            msg += padded_size; msg_len -= padded_size;
            EDEBUG("<- wl_output@%u.name: name=%.*s", display_state.wl_output, size, name);
            continue;
        }

        if(object_id == display_state.wl_output && opcode == wayland_wl_output_event_description) {
            u32 size = *(u32 *)msg;
            msg += sizeof(u32); msg_len -= sizeof(u32);
            u32 padded_size = roundup4(size);
            char description[255] = "";
            memcpy(description, msg, padded_size);
            msg += padded_size; msg_len -= padded_size;
            EDEBUG("<- wl_output@%u.description: description=%.*s", display_state.wl_output, size, description);
            continue;
        }

        if(object_id == display_state.wl_output && opcode == wayland_wl_output_event_scale) {
            u32 scale = *(u32 *)msg;
            msg += sizeof(u32); msg_len -= sizeof(u32);
            EDEBUG("<- wl_output@%u.scale: scale=%u", display_state.wl_output, scale);
            continue;
        }

        if(object_id == display_state.wl_output && opcode == wayland_wl_output_event_geometry) {
            u32 x = *(u32 *)msg;
            msg += sizeof(u32); msg_len -= sizeof(u32);
            u32 y = *(u32 *)msg;
            msg += sizeof(u32); msg_len -= sizeof(u32);
            u32 physical_width = *(u32 *)msg;
            msg += sizeof(u32); msg_len -= sizeof(u32);
            u32 physical_height = *(u32 *)msg;
            msg += sizeof(u32); msg_len -= sizeof(u32);
            u32 subpixel = *(u32 *)msg;
            msg += sizeof(u32); msg_len -= sizeof(u32);
            u32 make_size = *(u32 *)msg;
            msg += sizeof(u32); msg_len -= sizeof(u32);
            u32 make_padded_size = roundup4(make_size);
            char make[255] = "";
            memcpy(make, msg, make_padded_size);
            msg += make_padded_size; msg_len -= make_padded_size;
            u32 model_size = *(u32 *)msg;
            msg += sizeof(u32); msg_len -= sizeof(u32);
            u32 model_padded_size = roundup4(model_size);
            char model[255] = "";
            memcpy(model, msg, model_padded_size);
            msg += model_padded_size; msg_len -= model_padded_size;
            u32 transform = *(u32 *)msg;
            msg += sizeof(u32); msg_len -= sizeof(u32);
            EDEBUG("<- wl_output@%u.geometry: x=%u y=%u physical_width=%u physical_height=%u subpixel=%u make=%.*s model=%.*s transform=%u", display_state.wl_output, x, y, physical_width, physical_height, subpixel, make_size, make, model_size, model, transform);
            continue;
        }

        if(object_id == display_state.wl_output && opcode == wayland_wl_output_event_done) {
            EDEBUG("<- wl_output@%u.done", display_state.wl_output);
            continue;
        }

        if(object_id == display_state.wl_output && opcode == wayland_wl_output_event_mode) {
            u32 flags = *(u32 *)msg;
            msg += sizeof(u32); msg_len -= sizeof(u32);
            u32 width = *(u32 *)msg;
            msg += sizeof(u32); msg_len -= sizeof(u32);
            u32 height = *(u32 *)msg;
            msg += sizeof(u32); msg_len -= sizeof(u32);
            u32 refresh = *(u32 *)msg;
            msg += sizeof(u32); msg_len -= sizeof(u32);
            EDEBUG("<- wl_output@%u.mode: flags=0x%x w=%u h=%u refresh=%u", display_state.wl_output, flags, width, height, refresh);
            continue;
        }

        if(object_id == wayland_display_object_id && opcode == wayland_wl_display_event_delete_id) {
            u32 id = *(u32 *)msg;
            msg += sizeof(u32); msg_len -= sizeof(u32);

            // check if the object is a pool
            u32 i = 0;
            u32 len = sizeof(display_state.old_memchunks) / sizeof(memchunk);
            while(i < len && display_state.old_memchunks[i].wl_object != id) ++i;
            if(i < len) {
                // free the memory associated with the deleted pool
                memchunk *chunk = &display_state.old_memchunks[i];
                window_unalloc_memory(chunk->size, chunk->data, chunk->fd);
                chunk->wl_object = 0;
                chunk->wl_buffers[0] = 0; // TODO: account for double buffering
                chunk->fd = 0;
                chunk->data = 0;
                chunk->size = 0;
                EDEBUG("freed memory linked to wl_shm_pool@%u", id);
            }

            EDEBUG("<- wl_display@%u.delete_id: id=%u", wayland_display_object_id, id);
            continue;
        }

        if(object_id == wayland_display_object_id && opcode == wayland_wl_display_error_event) {
            u32 target_object_id = *(u32 *)msg;
            msg += sizeof(u32); msg_len -= sizeof(u32);
            u32 code = *(u32 *)msg;
            msg += sizeof(u32); msg_len -= sizeof(u32);
            char error[512] = "";
            u32 error_len = *(u32 *)msg;
            msg += sizeof(u32); msg_len -= sizeof(u32);
            u32 padded_error_len = roundup4(error_len);
            memcpy(error, msg, padded_error_len);
            msg += padded_error_len; msg_len -= padded_error_len;

            EFATAL("error from Wayland: target_object_id=%u code=%u error=%s", target_object_id, code, error);
            EASSERT(false);
            return false;
        }

        // check if event comes from an old buffer
        if(opcode == wayland_wl_buffer_event_release) {
            u32 i = 0;
            u32 len = sizeof(display_state.old_memchunks) / sizeof(memchunk);
            while(i < len && display_state.old_memchunks[i].wl_buffers[0] != object_id) ++i;
            if(i < len) {
                EDEBUG("<- wl_buffer@%u.release", display_state.old_memchunks[i].wl_buffers[0]);
                continue;
            }
        }

        // dispatch event to each window backend

        u8 consumed = false;
        darray_foreach(ewindow, window, &display_state.windows) {
            // we will feed the event to one window, then pass
            // it to the next ones if it has not been consumed 
            if(ewindow_pump(window, object_id, opcode, &msg, &msg_len)) {
                consumed = true;
                break;
            }
        }
        if(consumed) {
            continue;
        }

        // event not consumed by any window

        EFATAL("unimplemented Wayland event: object_id=%u opcode=%u", object_id, opcode);
        EASSERT(false);
        return false;
    }

    // TODO: tmp
    darray_foreach(ewindow, window, &display_state.windows) {
        window_render(window->backend_state);
    }

    return true;
}

static u8 ewindow_pump(ewindow *window, u32 object_id, u16 opcode, u8 **msg, u64 *msg_len) {
    EASSERT(window != 0);

    window_backend_state *backend_state;
    backend_state = window->backend_state;

    if(object_id == backend_state->wl_buffer && opcode == wayland_wl_buffer_event_release) {
        EDEBUG("<- wl_buffer@%u.release", backend_state->wl_buffer);
        return true;
    }

    if(window && object_id == backend_state->xdg_toplevel && opcode == wayland_xdg_toplevel_event_wm_capabilities) {
        u32 len = *(u32 *)(*msg);
        *msg += sizeof(u32); *msg_len -= sizeof(u32);
        char buf[256] = "";
        EASSERT(len <= sizeof(buf));
        memcpy(buf, *msg, len);
        *msg += len; *msg_len -= len;
        EDEBUG("<- xdg_toplevel@%u.wm_capabilities: capabilities[%u]", backend_state->xdg_toplevel, len);
        return true;
    }

    if(window && object_id == backend_state->xdg_surface && opcode == wayland_xdg_surface_event_configure) {
        u32 serial = *(u32 *)(*msg);
        *msg += sizeof(u32); *msg_len -= sizeof(u32);

        EDEBUG("<- xdg_surface@%u.configure: serial=%u", backend_state->xdg_surface, serial);

        if((backend_state->width_req != 0 && backend_state->height_req != 0) 
                && (backend_state->width_req != backend_state->width || backend_state->height_req != backend_state->height)) {
            window_unbind_memory(backend_state);

            // update infos and allocate new memory
            backend_state->width = backend_state->width_req;
            backend_state->height = backend_state->height_req;
            backend_state->shm_pool_size = backend_state->width * backend_state->height * color_channels;
            window_alloc_memory(backend_state->shm_pool_size, (void *)&backend_state->shm_pool_data, &backend_state->shm_fd);
        }

        wayland_xdg_surface_ack_configure(backend_state, serial);

        // pool and buffers will be recreated next iteration
        backend_state->state = WINDOW_STATE_SURFACE_ACKED_CONFIGURE;

        return true;
    }

    if(object_id == backend_state->xdg_toplevel && opcode == wayland_xdg_toplevel_event_configure) {
        u32 w = *(u32 *)(*msg);
        *msg += sizeof(u32); *msg_len -= sizeof(u32);
        u32 h = *(u32 *)(*msg);
        *msg += sizeof(u32); *msg_len -= sizeof(u32);
        u32 len = *(u32 *)(*msg);
        *msg += sizeof(u32); *msg_len -= sizeof(u32);
        char buf[256] = "";
        EASSERT(len <= sizeof(buf));
        memcpy(buf, *msg, len);
        *msg += len; *msg_len -= len;

        if((w != 0 && h != 0) && (w != backend_state->width || h != backend_state->height)) {
            // change dimensions, will reallocate memory when acking the configure event
            backend_state->width_req = w;
            backend_state->height_req = h;
        }

        EDEBUG("<- xdg_toplevel@%u.configure: w=%u h=%u states[%u]", backend_state->xdg_toplevel, w, h, len);
        return true;
    }

    if(object_id == backend_state->wl_surface && opcode == wayland_wl_surface_event_preferred_buffer_scale) {
        i32 factor = *(i32 *)(*msg);
        *msg += sizeof(i32); *msg_len -= sizeof(i32);
        EDEBUG("<- wl_surface@%u.preferred_buffer_scale: factor=%u", backend_state->wl_surface, factor);
        return true;
    }

    if(object_id == backend_state->wl_surface && opcode == wayland_wl_surface_event_preferred_buffer_transform) {
        u32 transform = *(u32 *)(*msg);
        *msg += sizeof(u32); *msg_len -= sizeof(u32);
        EDEBUG("<- wl_surface@%u.preferred_buffer_transform: transform=%u", backend_state->wl_surface, transform);
        return true;
    }

    if(object_id == backend_state->xdg_toplevel && opcode == wayland_xdg_toplevel_event_close) {
        EDEBUG("<- xdg_toplevel@%u.close", backend_state->xdg_toplevel);
        backend_state->state = WINDOW_STATE_SHOULD_CLOSE;
        return true;
    }

    if(object_id == backend_state->wl_surface && opcode == wayland_wl_surface_event_enter) {
        u32 output = *(u32 *)(*msg);
        *msg += sizeof(u32); *msg_len -= sizeof(u32);
        EDEBUG("<- wl_surface@%u.enter: output=%u", backend_state->wl_surface, output);
        return true;
    }

    // message not consumed 
    return false;
}

static ewindow *get_window(u64 window_id) {
    darray_foreach(ewindow, window, &display_state.windows) {
        if(window->id == window_id) {
            return window;
        }
    }
    return NULL;
}

static u8 window_render(window_backend_state *backend_state) {
    // TODO: per window state
    if(backend_state->state == WINDOW_STATE_SURFACE_ACKED_CONFIGURE) {
        EASSERT(backend_state->wl_surface > 0);
        EASSERT(backend_state->xdg_surface > 0);
        EASSERT(backend_state->xdg_toplevel > 0);

        if(backend_state->wl_shm_pool == 0) {
            wayland_wl_shm_create_pool(backend_state);
            // TODO: return to pump errrors, but recv blocks for now
        }
        if(backend_state->wl_buffer == 0) {
            wayland_wl_shm_pool_create_buffer(backend_state);
            // TODO: return to pump errrors, but recv blocks for now
        }

        EASSERT(backend_state->shm_pool_data != 0);
        EASSERT(backend_state->shm_pool_size != 0);

        u32 *pixels = (u32 *)backend_state->shm_pool_data;
        for(u32 i = 0; i < backend_state->width * backend_state->height; ++i) {
            u8 r = 0xFF;
            u8 g = 0xFF;
            u8 b = 0x00;
            pixels[i] = (r << 16) | (g << 8) | b;
        }

        wayland_wl_surface_attach(backend_state);
        wayland_wl_surface_commit(backend_state);

        backend_state->state = WINDOW_STATE_SURFACE_ATTACHED;
    }

    return true;
}

u8 ewindow_should_close(u64 window_id) {
    ewindow *window = get_window(window_id);
    EASSERT(window != NULL);

    return window->backend_state->state == WINDOW_STATE_SHOULD_CLOSE;
}

static u8 window_alloc_memory(u32 size, void **shm_pool_data, i32 *shm_fd) {
    // create shared memory with Wayland compositor

    char name[255] = "/";
    for(u8 i = 1; i < sizeof(name) - 1; ++i) {
        name[i] = ((double)rand()) / (double)RAND_MAX * 26 + 'a';
    }
    
    i32 fd = shm_open(name, O_RDWR | O_EXCL | O_CREAT, 0600);
    if(fd == -1) {
        EERROR("failed to create shared memory during window creation");
        return false;
    }

    EASSERT(shm_unlink(name) != -1);

    if(ftruncate(fd, size) == -1) {
        EERROR("failed to truncate shared memory file");
        return false;
    }

    void *data = emap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    EASSERT(data != (void *)-1);
    EASSERT(data != NULL);

    *shm_pool_data = data;
    *shm_fd = fd;

    return true;
}

static u8 window_unalloc_memory(u32 size, void *shm_pool_data, i32 shm_fd) {
    eunmap(shm_pool_data, size);
    close(shm_fd);
    return true;
}

static u8 window_unbind_memory(window_backend_state *backend_state) {
    // if memory wasn't yet announced to the compositor, we can free immediately
    if(backend_state->wl_shm_pool == 0) {
        window_unalloc_memory(backend_state->shm_pool_size, backend_state->shm_pool_data, backend_state->shm_fd);
        return true;
    }

    // store old memory to free later
    u32 i = 0;
    while(display_state.old_memchunks[i].wl_object != 0) ++i;
    EASSERT(i < sizeof(display_state.old_memchunks) / sizeof(memchunk));
    memchunk *chunk = &display_state.old_memchunks[i];
    chunk->wl_object = backend_state->wl_shm_pool;
    chunk->wl_buffers[0] = backend_state->wl_buffer; // TODO: account for double buffering
    chunk->fd = backend_state->shm_fd;
    chunk->data = backend_state->shm_pool_data;
    chunk->size = backend_state->shm_pool_size;

    // send delete pool and buffer(s) requests if exist
    // we will wait for delete_id event before freeing the actual memory
    if(backend_state->wl_buffer > 0) {
        wayland_wl_buffer_destroy(backend_state);
    }
    if(backend_state->wl_shm_pool > 0) {
        wayland_wl_shm_pool_destroy(backend_state);
    }

    return true;
}

static u8 window_create_surface(window_backend_state *backend_state) {
    if(backend_state->wl_surface == 0) {
        // TODO: assert that display_backend is init
        wayland_wl_compositor_create_surface(backend_state);
        wayland_xdg_wm_base_get_xdg_surface(backend_state);
        wayland_xdg_surface_get_toplevel(backend_state);
        wayland_wl_surface_commit(backend_state);

        return true;
    }

    return false;
}

static u8 window_destroy_surface(window_backend_state *backend_state) {
    if(backend_state->wl_surface != 0) {
        wayland_xdg_toplevel_destroy(backend_state);
        wayland_xdg_surface_destroy(backend_state);
        wayland_wl_surface_destroy(backend_state);

        return true;
    }

    return false;
}

static u8 wayland_wl_registry_bind(u32 name, char *interface, u32 interface_len, u32 version, u32 *state_interface) {
    u8 msg[512] = "";
    u64 msg_idx = 0;
    u16 padded_interface_len = roundup4(interface_len);

    write_u32(msg, &msg_idx, display_state.wl_registry);

    write_u16(msg, &msg_idx, wayland_wl_registry_bind_opcode);

    u16 final_msg_size = wayland_header_size + sizeof(name) + sizeof(interface_len) + padded_interface_len + sizeof(version) + sizeof(display_state.current_id);
    EASSERT(roundup4(final_msg_size) == final_msg_size);

    write_u16(msg, &msg_idx, final_msg_size);

    write_u32(msg, &msg_idx, name);

    write_u32(msg, &msg_idx, interface_len);

    memcpy(msg + msg_idx, interface, padded_interface_len);
    msg_idx += padded_interface_len;

    write_u32(msg, &msg_idx, version);

    write_u32(msg, &msg_idx, ++display_state.current_id);
    EASSERT(roundup4(msg_idx) == final_msg_size);

    if(msg_idx != send(display_state.fd, msg, final_msg_size, 0)) {
        EERROR("failed to bind Wayland interface: %s", interface);
        return false;
    }

    *state_interface = display_state.current_id;

    EDEBUG("bound Wayland interface: -> wl_registry@%u.bind: name=%u interface=%.*s version=%u", display_state.wl_registry, name, interface_len, interface, version);

    return true;
}

static u8 wayland_wl_seat_get_keyboard() {
    EASSERT(display_state.wl_seat > 0);

    u8 msg[128] = "";
    u64 msg_idx = 0;

    write_u32(msg, &msg_idx, display_state.wl_seat);

    write_u16(msg, &msg_idx, wayland_wl_seat_get_keyboard_opcode);

    u16 final_msg_size = wayland_header_size + sizeof(display_state.current_id);
    EASSERT(roundup4(final_msg_size) == final_msg_size);

    write_u16(msg, &msg_idx, final_msg_size);

    write_u32(msg, &msg_idx, ++display_state.current_id);

    if(msg_idx != send(display_state.fd, msg, final_msg_size, 0)) {
        EERROR("failed to get keyboard from Wayland");
        return false;
    }

    display_state.wl_keyboard = display_state.current_id;

    EDEBUG("got keyboard: -> wl_seat@%u.get_keyboard: keyboard=%u", display_state.wl_seat, display_state.wl_keyboard);

    return true;
}

static u8 wayland_wl_shm_create_pool(window_backend_state *backend_state) {
    EASSERT(backend_state->shm_pool_size > 0);

    u8 msg[128] = "";
    u64 msg_idx = 0;

    write_u32(msg, &msg_idx, display_state.wl_shm);

    write_u16(msg, &msg_idx, wayland_wl_shm_create_pool_opcode);

    u16 final_msg_size = wayland_header_size + sizeof(display_state.current_id) + sizeof(backend_state->shm_pool_size);
    EASSERT(roundup4(final_msg_size) == final_msg_size);

    write_u16(msg, &msg_idx, final_msg_size);

    write_u32(msg, &msg_idx, ++display_state.current_id);

    write_u32(msg, &msg_idx, backend_state->shm_pool_size);

    EASSERT(msg_idx == final_msg_size);

    char buf[CMSG_SPACE(sizeof(backend_state->shm_fd))];

    struct iovec io = { .iov_base = msg, .iov_len = msg_idx };
    struct msghdr socket_msg = {
        .msg_iov = &io,
        .msg_iovlen = 1,
        .msg_control = buf,
        .msg_controllen = sizeof(buf),
    };

    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&socket_msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(backend_state->shm_fd));

    memcpy(CMSG_DATA(cmsg), &backend_state->shm_fd, sizeof(backend_state->shm_fd));
    socket_msg.msg_controllen = cmsg->cmsg_len;

    if(sendmsg(display_state.fd, &socket_msg, 0) == -1) {
        EERROR("failed to share shm pool during window creation");
        return false;
    }

    backend_state->wl_shm_pool = display_state.current_id;

    EDEBUG("created shm pool: -> wl_shm@%u.create_pool: wl_shm_pool=%u", display_state.wl_shm, display_state.current_id);

    return true;
}

static u8 wayland_wl_shm_pool_create_buffer(window_backend_state *backend_state) {
    EASSERT(backend_state->wl_shm_pool > 0);

    u8 msg[128] = "";
    u64 msg_idx = 0;

    write_u32(msg, &msg_idx, backend_state->wl_shm_pool);

    write_u16(msg, &msg_idx, wayland_wl_shm_pool_create_buffer_opcode);

    u16 final_msg_size = wayland_header_size + sizeof(display_state.current_id) + sizeof(u32) * 5;
    EASSERT(roundup4(final_msg_size) == final_msg_size);

    write_u16(msg, &msg_idx, final_msg_size);

    write_u32(msg, &msg_idx, ++display_state.current_id);

    u32 offset = 0;
    write_u32(msg, &msg_idx, offset);

    write_u32(msg, &msg_idx, backend_state->width);

    write_u32(msg, &msg_idx, backend_state->height);

    write_u32(msg, &msg_idx, backend_state->width * color_channels);

    u32 format = wayland_format_xrgb8888;
    write_u32(msg, &msg_idx, format);

    EASSERT(msg_idx == final_msg_size);

    if(msg_idx != send(display_state.fd, msg, final_msg_size, 0)) {
        EERROR("failed to create Wayland buffer");
        return false;
    }

    backend_state->wl_buffer = display_state.current_id;

    EDEBUG("created Wayland buffer: -> wl_shm_pool@%u.create_buffer: wl_buffer=%u", backend_state->wl_shm_pool, backend_state->wl_buffer);

    return true;
}

static u8 wayland_wl_compositor_create_surface(window_backend_state *backend_state) {
    EASSERT(display_state.wl_compositor > 0);

    u8 msg[128] = "";
    u64 msg_idx = 0;

    write_u32(msg, &msg_idx, display_state.wl_compositor);

    write_u16(msg, &msg_idx, wayland_wl_compositor_create_surface_opcode);

    u16 final_msg_size = wayland_header_size + sizeof(display_state.current_id);
    EASSERT(roundup4(final_msg_size) == final_msg_size);

    write_u16(msg, &msg_idx, final_msg_size);

    write_u32(msg, &msg_idx, ++display_state.current_id);

    if(msg_idx != send(display_state.fd, msg, final_msg_size, 0)) {
        EERROR("failed to create Wayland surface");
        return false;
    }

    backend_state->wl_surface = display_state.current_id;

    EDEBUG("created Wayland surface: -> wl_compositor@%u.create_surface: wl_surface=%u", display_state.wl_compositor, backend_state->wl_surface);

    return true;
}

static u8 wayland_xdg_wm_base_get_xdg_surface(window_backend_state *backend_state) {
    EASSERT(display_state.xdg_wm_base > 0);
    EASSERT(backend_state->wl_surface > 0);

    u8 msg[128] = "";
    u64 msg_idx = 0;

    write_u32(msg, &msg_idx, display_state.xdg_wm_base);

    write_u16(msg, &msg_idx, wayland_xdg_wm_base_get_xdg_surface_opcode);

    u16 final_msg_size = wayland_header_size + sizeof(display_state.current_id) + sizeof(backend_state->wl_surface);
    EASSERT(roundup4(final_msg_size) == final_msg_size);

    write_u16(msg, &msg_idx, final_msg_size);

    write_u32(msg, &msg_idx, ++display_state.current_id);

    write_u32(msg, &msg_idx, backend_state->wl_surface);

    if(msg_idx != send(display_state.fd, msg, final_msg_size, 0)) {
        EERROR("failed to get xdg surface from Wayland");
        return false;
    }

    backend_state->xdg_surface = display_state.current_id;

    EDEBUG("got xdg surface: -> xdg_wm_base@%u.get_xdg_surface: xdg_surface=%u wl_surface=%u", display_state.xdg_wm_base, backend_state->xdg_surface, backend_state->wl_surface);

    return true;
}

static u8 wayland_xdg_surface_get_toplevel(window_backend_state *backend_state) {
    EASSERT(backend_state->xdg_surface > 0);

    u8 msg[128] = "";
    u64 msg_idx = 0;

    write_u32(msg, &msg_idx, backend_state->xdg_surface);

    write_u16(msg, &msg_idx, wayland_xdg_surface_get_toplevel_opcode);

    u16 final_msg_size = wayland_header_size + sizeof(display_state.current_id);
    EASSERT(roundup4(final_msg_size) == final_msg_size);

    write_u16(msg, &msg_idx, final_msg_size);

    write_u32(msg, &msg_idx, ++display_state.current_id);

    if(msg_idx != send(display_state.fd, msg, final_msg_size, 0)) {
        EERROR("failed to get xdg toplevel from Wayland");
        return false;
    }

    backend_state->xdg_toplevel = display_state.current_id;

    EDEBUG("got xdg toplevel: -> xdg_surface@%u.get_toplevel: xdg_toplevel=%u", backend_state->xdg_surface, backend_state->xdg_toplevel);

    return true;
}

static u8 wayland_wl_surface_commit(window_backend_state *backend_state) {
    EASSERT(backend_state->wl_surface > 0);

    u8 msg[128] = "";
    u64 msg_idx = 0;

    write_u32(msg, &msg_idx, backend_state->wl_surface);

    write_u16(msg, &msg_idx, wayland_wl_surface_commit_opcode);

    u16 final_msg_size = wayland_header_size;
    EASSERT(roundup4(final_msg_size) == final_msg_size);

    write_u16(msg, &msg_idx, final_msg_size);

    if(msg_idx != send(display_state.fd, msg, final_msg_size, 0)) {
        EERROR("failed to commit Wayland surface");
        return false;
    }

    EDEBUG("committed Wayland surface: -> wl_surface@%u.commit", backend_state->wl_surface);

    return true;
}

static u8 wayland_wl_surface_attach(window_backend_state *backend_state) {
    EASSERT(backend_state->wl_surface > 0);
    EASSERT(backend_state->wl_buffer > 0);

    u8 msg[128] = "";
    u64 msg_idx = 0;

    write_u32(msg, &msg_idx, backend_state->wl_surface);

    write_u16(msg, &msg_idx, wayland_wl_surface_attach_opcode);

    u16 final_msg_size = wayland_header_size + sizeof(backend_state->wl_buffer) + sizeof(u32) * 2;
    EASSERT(roundup4(final_msg_size) == final_msg_size);

    write_u16(msg, &msg_idx, final_msg_size);

    write_u32(msg, &msg_idx, backend_state->wl_buffer);

    u32 x = 0, y = 0;
    write_u32(msg, &msg_idx, x);
    write_u32(msg, &msg_idx, y);

    EASSERT(msg_idx == final_msg_size);

    int res = send(display_state.fd, msg, final_msg_size, 0);
    if(msg_idx != res) {
        EERROR("failed to attach Wayland surface");
        return false;
    }

    EDEBUG("attached Wayland surface: -> wl_surface@%u.attach: wl_buffer=%u", backend_state->wl_surface, backend_state->wl_buffer);

    return true;
}

static u8 wayland_xdg_wm_base_pong(u32 ping) {
    EASSERT(display_state.xdg_wm_base > 0);
    // EASSERT(display_state.wl_surface > 0);

    u8 msg[128] = "";
    u64 msg_idx = 0;

    write_u32(msg, &msg_idx, display_state.xdg_wm_base);

    write_u16(msg, &msg_idx, wayland_xdg_wm_base_pong_opcode);

    u16 final_msg_size = wayland_header_size + sizeof(ping);
    EASSERT(roundup4(final_msg_size) == final_msg_size);

    write_u16(msg, &msg_idx, final_msg_size);

    write_u32(msg, &msg_idx, ping);

    if(msg_idx != send(display_state.fd, msg, final_msg_size, 0)) {
        EERROR("failed to answer xdg_wm_base ping");
        return false;
    }

    EDEBUG("pong xdg wm base: -> xdg_wm_base@%u.pong: ping=%u", display_state.xdg_wm_base, ping);

    return true;
}

static u8 wayland_xdg_surface_ack_configure(window_backend_state *backend_state, u32 serial) {
    EASSERT(backend_state->xdg_surface > 0);

    u8 msg[128] = "";
    u64 msg_idx = 0;

    write_u32(msg, &msg_idx, backend_state->xdg_surface);

    write_u16(msg, &msg_idx, wayland_xdg_surface_ack_configure_opcode);

    u16 final_msg_size = wayland_header_size + sizeof(serial);
    EASSERT(roundup4(final_msg_size) == final_msg_size);

    write_u16(msg, &msg_idx, final_msg_size);

    write_u32(msg, &msg_idx, serial);

    if(msg_idx != send(display_state.fd, msg, final_msg_size, 0)) {
        EERROR("failed to acknowledge xdg_surface configure");
        return false;
    }

    EDEBUG("acked xdg_surface configure: -> xdg_surface@%u.ack_configure: serial=%u", backend_state->xdg_surface, serial);

    return true;
}

static u8 wayland_wl_buffer_destroy(window_backend_state *backend_state) {
    EASSERT(backend_state->wl_buffer > 0);

    u8 msg[128] = "";
    u64 msg_idx = 0;

    write_u32(msg, &msg_idx, backend_state->wl_buffer);

    write_u16(msg, &msg_idx, wayland_wl_buffer_destroy_opcode);

    u16 final_msg_size = wayland_header_size;
    EASSERT(roundup4(final_msg_size) == final_msg_size);

    write_u16(msg, &msg_idx, final_msg_size);

    if(msg_idx != send(display_state.fd, msg, final_msg_size, 0)) {
        EERROR("failed to destroy wl_buffer");
        return false;
    }

    EDEBUG("destroyed wl_buffer: -> wl_buffer@%u.destroy", backend_state->wl_buffer);

    backend_state->wl_buffer = 0;

    return true;
}

static u8 wayland_wl_shm_pool_destroy(window_backend_state *backend_state) {
    EASSERT(backend_state->wl_shm_pool > 0);

    u8 msg[128] = "";
    u64 msg_idx = 0;

    write_u32(msg, &msg_idx, backend_state->wl_shm_pool);

    write_u16(msg, &msg_idx, wayland_wl_shm_pool_destroy_opcode);

    u16 final_msg_size = wayland_header_size;
    EASSERT(roundup4(final_msg_size) == final_msg_size);

    write_u16(msg, &msg_idx, final_msg_size);

    if(msg_idx != send(display_state.fd, msg, final_msg_size, 0)) {
        EERROR("failed to destroy wl_shm_pool");
        return false;
    }

    EDEBUG("destroyed wl_shm_pool: -> wl_shm_pool@%u.destroy", backend_state->wl_shm_pool);

    backend_state->wl_shm_pool = 0;

    return true;
}

static u8 wayland_xdg_toplevel_destroy(window_backend_state *backend_state) {
    EASSERT(backend_state->xdg_toplevel > 0);

    u8 msg[128] = "";
    u64 msg_idx = 0;

    write_u32(msg, &msg_idx, backend_state->xdg_toplevel);

    write_u16(msg, &msg_idx, wayland_xdg_toplevel_destroy_opcode);

    u16 final_msg_size = wayland_header_size;
    EASSERT(roundup4(final_msg_size) == final_msg_size);

    write_u16(msg, &msg_idx, final_msg_size);

    if(msg_idx != send(display_state.fd, msg, final_msg_size, 0)) {
        EERROR("failed to destroy xdg_toplevel");
        return false;
    }

    EDEBUG("destroyed xdg_toplevel: -> xdg_toplevel@%u.destroy", backend_state->xdg_toplevel);

    backend_state->xdg_toplevel = 0;

    return true;
}

static u8 wayland_xdg_surface_destroy(window_backend_state *backend_state) {
    EASSERT(backend_state->xdg_surface > 0);

    u8 msg[128] = "";
    u64 msg_idx = 0;

    write_u32(msg, &msg_idx, backend_state->xdg_surface);

    write_u16(msg, &msg_idx, wayland_xdg_surface_destroy_opcode);

    u16 final_msg_size = wayland_header_size;
    EASSERT(roundup4(final_msg_size) == final_msg_size);

    write_u16(msg, &msg_idx, final_msg_size);

    if(msg_idx != send(display_state.fd, msg, final_msg_size, 0)) {
        EERROR("failed to destroy xdg_surface");
        return false;
    }

    EDEBUG("destroyed xdg_surface: -> xdg_surface@%u.destroy", backend_state->xdg_surface);

    backend_state->xdg_surface = 0;

    return true;
}

static u8 wayland_wl_surface_destroy(window_backend_state *backend_state) {
    EASSERT(backend_state->wl_surface > 0);

    u8 msg[128] = "";
    u64 msg_idx = 0;

    write_u32(msg, &msg_idx, backend_state->wl_surface);

    write_u16(msg, &msg_idx, wayland_wl_surface_destroy_opcode);

    u16 final_msg_size = wayland_header_size;
    EASSERT(roundup4(final_msg_size) == final_msg_size);

    write_u16(msg, &msg_idx, final_msg_size);

    if(msg_idx != send(display_state.fd, msg, final_msg_size, 0)) {
        EERROR("failed to destroy wl_surface");
        return false;
    }

    EDEBUG("destroyed wl_surface: -> wl_surface@%u.destroy", backend_state->wl_surface);

    backend_state->wl_surface = 0;

    return true;
}

static void write_u32(u8 *buf, u64 *buf_idx, u32 val) {
    *((u32 *)(buf + *buf_idx)) = val;
    *buf_idx += sizeof(u32);
}

static void write_u16(u8 *buf, u64 *buf_idx, u16 val) {
    *((u16 *)(buf + *buf_idx)) = val;
    *buf_idx += sizeof(u16);
}
