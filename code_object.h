#pragma once
#include <stdio.h>
#include "property.h"
#include "drawing.h"

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

static void empty(code_object* self) {}

void draw_sprite(code_object* self, HDC display)
{
    draw_sprite_from_path(self->sprite_path, display);
}

code_object default_code_object = { .create = empty, .step = empty, .draw = draw_sprite, .destroy = empty };