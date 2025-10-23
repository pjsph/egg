#ifndef WINDOW_H
#define WINDOW_H

#include "defines.h"

typedef struct ewindow {
    const char* title;
    u16 width;
    u16 height;
    const char *icon;
} ewindow;

EAPI u8 ewindow_create(ewindow *window);
EAPI u8 ewindow_should_close();

#endif // WINDOW_H
