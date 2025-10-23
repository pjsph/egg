#include "engine.h"

#include "window.h"

#include <stdio.h>

void engine_run(eapp *app) {
    printf("Hello from lib!\n");

    ewindow_create(&app->window);

    app->init(app);

    while(!ewindow_should_close()) {
        app->update(app);
        app->render(app);
    }
}
