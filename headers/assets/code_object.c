#include <stdio.h>
#include "../drawing.h"
#include "./code_object.h"

static void empty(code_object* self) {}

void draw_code_object_sprite(code_object* self, HDC display)
{
    draw_sprite(display, self->sprite_path, self->x, self->y);
}

code_object default_code_object = { .create = empty, .step = empty, .draw = draw_code_object_sprite, .destroy = empty };