#include <stdio.h>

#include "app.h"
#include "engine.h"

extern void app_new(eapp* app);

int main(void) {
    printf("Hello, world!\n");
    eapp app = {0};
    app_new(&app);
    INIT();
    engine_run(&app);
    return 0;
}
