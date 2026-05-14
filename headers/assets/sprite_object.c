#include <stdio.h>
#include "../property.h"
#include "../drawing.h"
#include "./sprite_object.h"

void draw_sprite_object_sprite(sprite_object* self, HDC display)
{
    draw_sprite_from_path(self->sprite_path, display);
}

sprite_object default_sprite_object = {};