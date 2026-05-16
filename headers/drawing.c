#ifndef UNICODE
#define UNICODE
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <windows.h>
#include <wingdi.h>
#include "./drawing.h"

bool draw_pixels(HDC display, uint32_t* pixels, int x, int y, int width, int height)
{
    if (!pixels) { return 0; }

    BITMAPINFO bitmap_info = {0};
    bitmap_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmap_info.bmiHeader.biWidth = width;
    bitmap_info.bmiHeader.biHeight = -height;
    bitmap_info.bmiHeader.biPlanes = 1;
    bitmap_info.bmiHeader.biBitCount = 8 * sizeof(uint32_t);
    bitmap_info.bmiHeader.biCompression = BI_RGB;
    
    uint32_t* supply_pixels = NULL;
    HBITMAP bitmap = CreateDIBSection(display, &bitmap_info, DIB_RGB_COLORS, (void**)&supply_pixels, NULL, 0);
    memcpy_s(supply_pixels, width * height * sizeof(uint32_t), pixels, width * height * sizeof(uint32_t));

    HDC src = CreateCompatibleDC(NULL);
    HBITMAP old_bitmap = SelectObject(src, bitmap);

    BLENDFUNCTION blendmode = {};
    blendmode.BlendOp = AC_SRC_OVER;
    blendmode.BlendFlags = 0;
    blendmode.SourceConstantAlpha = 255;
    blendmode.AlphaFormat = AC_SRC_ALPHA;

    //Though pixels are supposed to be BGRA, because of little endian they can be written into uint32s as RGBA
    bool success = AlphaBlend
    (
        display, x, y, width, height,
        src, 0, 0, width, height,
        blendmode
    );

    SelectObject(src, old_bitmap);
    DeleteDC(src);
    DeleteObject(bitmap);
    return success;
}