#pragma once
#include "./code_object.h"
#include "./sprite_object.h"

typedef struct room_object room_object;
struct room_object
{
    code_object* code_objects;
    sprite_object* sprite_objects;
};

extern room_object default_room_object;