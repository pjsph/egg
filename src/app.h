#ifndef APP_H
#define APP_H

#include "defines.h"
#include "window.h"

typedef struct eapp {
    u64 window;

    u8 (*init)(struct eapp *app);

    u8 (*update)(struct eapp* app);
    u8 (*render)(struct eapp* app);
} eapp;

#endif // APP_H
