#ifndef UNICODE
#define UNICODE
#endif

#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <windows.h>
#include "./file_reader.h"

extern int draw_pixels(HDC display, uint32_t* pixels, int x, int y, int width, int height);
extern void draw_sprite(HDC display, char* sprite_path, int x, int y);