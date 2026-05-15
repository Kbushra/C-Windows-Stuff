#ifndef UNICODE
#define UNICODE
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <windows.h>
#include "./drawing.h"
#include "./file_reader.h"

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