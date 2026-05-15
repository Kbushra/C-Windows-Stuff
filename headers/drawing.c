#ifndef UNICODE
#define UNICODE
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <windows.h>
#include "./drawing.h"
#include "./file_reader.h";

int draw_pixels(HDC display, uint32_t* pixels, int x, int y, int width, int height)
{
    BITMAPINFO bitmap =
    {
        .bmiHeader =
        {
            sizeof(BITMAPINFOHEADER),
            width,
            -height,
            1,
            sizeof(uint32_t) * 8,
            BI_RGB
        }
    };

    return StretchDIBits
    (
        display,
        x, y, width, height,
        0, 0, width, height,
        (void*)pixels, &bitmap,
        DIB_RGB_COLORS, SRCCOPY
    );
}

void draw_sprite(HDC display, char* sprite_path, int x, int y)
{
    FILE* file = fopen(sprite_path, "r");
    if (!file) { return; }

    int width = 0;
    int height = 0;
    uint32_t* pixels = read_png_file(file, &width, &height);
    draw_pixels(display, pixels, x, y, width, height);
}