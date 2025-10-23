#ifndef RENDERER_H
#define RENDERER_H

#include "defines.h"

EAPI u8 renderer_clear();
EAPI u8 renderer_draw_asset(u32 id, int x, int y, int width, int height);

#endif // RENDERER_H

