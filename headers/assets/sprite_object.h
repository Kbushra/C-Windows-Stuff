#ifndef UNICODE
#define UNICODE
#endif

#pragma once
#include <stdint.h>
#include "../drawing.h"

typedef struct sprite_object sprite_object;
struct sprite_object
{
    int x;
    int y;
    int sprite_width;
    int sprite_height;
    uint32_t* sprite_pixels;
};

extern sprite_object default_sprite_object;