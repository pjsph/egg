#include "src/engine.h"
#include "src/window.h"
#include "src/app.h"
#include "src/defines.h"
#include "src/logger.h"

#include <stdio.h>

u8 update(eapp *app) {
    return false;
}

u8 render(eapp *app) {
    return true;
}

// we can define variables that have a static lifetime here
#define INIT()


u8 init(eapp *app) {
    return true;
}

void app_new(eapp *app) {
    app->init = init;
    app->update = update;
    app->render = render;

    EINFO("created app");
    EDEBUG("this is a debug note");
}

#include "src/entry.h"
