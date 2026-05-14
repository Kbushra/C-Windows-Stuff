#pragma once
#include <stdio.h>

typedef struct property property;
struct property
{
    char* name;
    void* value;
};