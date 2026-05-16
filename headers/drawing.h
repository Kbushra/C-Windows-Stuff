#ifndef UNICODE
#define UNICODE
#endif

#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <windows.h>
#include <wingdi.h>

extern bool draw_pixels(HDC display, uint32_t* pixels, int x, int y, int width, int height);