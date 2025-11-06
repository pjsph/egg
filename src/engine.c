#include "engine.h"

#include "window.h"
#include "logger.h"

#include <stdlib.h>
#include <time.h>

// extern u32 display_backend_size;

void engine_run(eapp *app) {
    EINFO("Hello from lib!");

    // init random system
    srand(time(NULL));

    // struct display_backend_state *display_backend = malloc(display_backend_size);

    if(!display_backend_init(0)) {
        EFATAL("ERROR: failed to initialize display system. Crashing");
        return;
    }

    ewindow_config config = { .x = 100, .y = 100, .width = 600, .height = 400, .title = NULL };
    ewindow_create(&config, &app->window);

    ewindow_config config2 = { .x = 100, .y = 100, .width = 600, .height = 400, .title = NULL };
    u64 win_id;
    ewindow_create(&config2, &win_id);

    app->init(app);

    EDEBUG("here");

    while(!ewindow_should_close(app->window)) {
        ewindow_pump_all();
        app->update(app);
        app->render(app);
    }

    ewindow_destroy(app->window);
    ewindow_destroy(app->window);
}
