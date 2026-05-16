#ifndef UNICODE
#define UNICODE
#endif

#pragma once
#include <stdint.h>
#include <windows.h>
#include "../drawing.h"
#include "../property.h"

typedef struct code_object code_object;
struct code_object
{
    int x;
    int y;
    int sprite_width;
    int sprite_height;
    uint32_t* sprite_pixels;
    property* props;
    
    void (*create)(code_object* self);
    void (*step)(code_object* self);
    void (*draw)(code_object* self, HDC display);
    void (*destroy)(code_object* self);
};

extern void draw_code_object_sprite(code_object* self, HDC display);
extern code_object default_code_object;