#ifndef UNICODE
#define UNICODE
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <windows.h>
#include "./drawing.h"

int draw_pixels(HDC display, uint32_t* pixels, int x, int y, int width, int height)
{
    if (!pixels) { return 0; }

    BITMAPINFO bitmap = {0};
    bitmap.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmap.bmiHeader.biWidth = width;
    bitmap.bmiHeader.biHeight = -height;
    bitmap.bmiHeader.biPlanes = 1;
    bitmap.bmiHeader.biBitCount = 8 * sizeof(uint32_t);
    bitmap.bmiHeader.biCompression = BI_RGB;
    
    return StretchDIBits
    (
        display,
        x, y, width, height,
        0, 0, width, height,
        (void*)pixels, &bitmap,
        DIB_RGB_COLORS, SRCCOPY
    );
}