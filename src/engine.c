#include "engine.h"

#include "window.h"
#include "logger.h"

#include <stdlib.h>

// extern u32 display_backend_size;

void engine_run(eapp *app) {
    EINFO("Hello from lib!");

    // struct display_backend_state *display_backend = malloc(display_backend_size);

    if(!display_backend_init(0)) {
        EFATAL("ERROR: failed to initialize display system. Crashing");
        return;
    }

    ewindow_config config = { .x = 100, .y = 100, .width = 600, .height = 400, .title = NULL };
    ewindow_create(&config, &app->window);

    app->init(app);

    while(!ewindow_should_close()) {
        ewindow_pump(&app->window);
        app->update(app);
        app->render(app);
    }

    ewindow_destroy(&app->window);
}
