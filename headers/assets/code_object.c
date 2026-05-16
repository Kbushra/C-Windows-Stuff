#include <stdint.h>
#include <windows.h>
#include "../drawing.h"
#include "../property.h"
#include "./code_object.h"

static void empty(code_object* self) {}

void draw_code_object_sprite(code_object* self, HDC display)
{
    draw_pixels(display, self->sprite_pixels, self->x, self->y, self->sprite_width, self->sprite_height);
}

code_object default_code_object =
{
    .x = 0,
    .y = 0,
    .sprite_width = 0,
    .sprite_height = 0,
    .sprite_pixels = NULL,

    .create = empty,
    .step = empty,
    .draw = draw_code_object_sprite,
    .destroy = empty
};