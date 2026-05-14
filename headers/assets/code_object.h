#pragma once
#include <stdio.h>
#include "../property.h"
#include "../drawing.h"

typedef struct code_object code_object;
struct code_object
{
    int x;
    int y;
    char* sprite_path;
    property* props;
    
    void (*create)(code_object* self);
    void (*step)(code_object* self);
    void (*draw)(code_object* self, HDC display);
    void (*destroy)(code_object* self);
};

extern void draw_code_object_sprite(code_object* self, HDC display);
extern code_object default_code_object;