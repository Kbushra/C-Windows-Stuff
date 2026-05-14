#ifndef UNICODE
#define UNICODE
#endif

#pragma once
#include <stdio.h>
#include <stdint.h>
#include <windows.h>

extern int draw_pixels(HDC display, uint32_t* pixels, int x, int y, int width, int height);
extern void draw_sprite_from_path(char* sprite_path, HDC display);