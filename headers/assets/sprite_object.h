#ifndef UNICODE
#define UNICODE
#endif

#pragma once
#include <stdio.h>
#include <windows.h>

typedef struct sprite_object sprite_object;
struct sprite_object
{
    int x;
    int y;
    char* sprite_path;
};

extern void draw_sprite_object_sprite(sprite_object* self, HDC display);