#include "engine.h"

#include "window.h"
#include "logger.h"
#include "heap.h"
#include "memory.h"

#include <stdlib.h>
#include <time.h>
#include <sys/mman.h>

// tmp
#define MEMORY_SIZE (1 * 1024 * 1024 * 1024)

void engine_run(eapp *app) {
    EINFO("Hello from lib!");

    // init random system
    srand(time(NULL));

    // init memory system
    eheap heap = {0};
    ememory_init(MEMORY_SIZE, &heap);
    ememory_report();

    if(!display_backend_init(0)) {
        EFATAL("ERROR: failed to initialize display system. Crashing");
        return;
    }

    ewindow_config config = { .x = 100, .y = 100, .width = 600, .height = 400, .title = NULL };
    ewindow_create(&config, &app->window);

    app->init(app);

    while(!ewindow_should_close(app->window)) {
        ewindow_pump_all();
        app->update(app);
        app->render(app);
    }

    // not pumped so memory is not cleaned
    ewindow_destroy(app->window);

    ememory_report();
    ememory_uninit();
}
