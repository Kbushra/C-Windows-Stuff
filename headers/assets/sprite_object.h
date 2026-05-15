#ifndef UNICODE
#define UNICODE
#endif

#pragma once
#include "../drawing.h"
#include <windows.h>

typedef struct sprite_object sprite_object;
struct sprite_object
{
    int x;
    int y;
    char* sprite_path;
};

extern sprite_object default_sprite_object;