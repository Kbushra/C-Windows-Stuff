#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <zlib.h>
#include <math.h>
#include "./file_loader.h"
#include "./debug.h"

#define GRAYSCALE 0
#define TRUECOLOUR 2
#define INDEXED_PALETTE 3
#define GRAYSCALE_ALPHA 4
#define TRUECOLOUR_ALPHA 6

#define FILTER_NONE 0
#define FILTER_SUB 1
#define FILTER_UP 2
#define FILTER_AVERAGE 3
#define FILTER_PAETH 4

#define INTERLACING_NONE 0
#define INTERLACING_ADAM 1

static uint32_t crc_table[256];
static bool generated_crc = false;

static void generate_crc_table(void)
{
    const uint32_t polynomial_coefficients = 0xEDB88320;

    for (int i = 0; i < 256; i++)
    {
        uint32_t crc_output = i;
        for (int j = 0; j < 8; j++)
        {
            //Losing a bit after shift
            if (crc_output & 1) { crc_output = (crc_output >> 1) ^ polynomial_coefficients; }
            else { crc_output >>= 1; }
        }

        crc_table[i] = crc_output;
    }

    generated_crc = true;
}

static bool valid_crc(uint8_t* buff, int len, uint32_t crc)
{
    uint32_t crc_output = 0xFFFFFFFF;
    if (!generated_crc) { generate_crc_table(); }

    for (int i = 0; i < len; i++) { crc_output = crc_table[ (crc_output ^ buff[i]) & 0xFF ] ^ (crc_output >> 8); }
    crc_output ^= 0xFFFFFFFF;

    return crc_output == crc;
}

static int paeth(uint8_t left, uint8_t up, uint8_t up_left)
{
    const int prediction = left + up - up_left;
    const int left_diff = abs(left - prediction);
    const int up_diff = abs(up - prediction);
    const int up_left_diff = abs(up_left - prediction);

    if (left_diff < up_diff && left_diff < up_left_diff) { return left; }
    else if (up_diff < up_left_diff) { return up; }
    else { return up_left; }
}

static uint32_t reverse_bytes(const uint32_t n)
{
    uint32_t byte1 = (n & 0x000000FF) << 24;
    uint32_t byte2 = (n & 0x0000FF00) << 8;
    uint32_t byte3 = (n & 0x00FF0000) >> 8;
    uint32_t byte4 = (n & 0xFF000000) >> 24;
    return byte1 | byte2 | byte3 | byte4;
}

static void read_word(uint32_t* dest, uint8_t* src, int* tracker)
{
    memcpy_s(dest, sizeof(uint32_t), src + *tracker, 4);
    *dest = reverse_bytes(*dest);
    *tracker += 4;
}

static void read_byte(uint8_t* dest, uint8_t* src, int* tracker)
{
    memcpy_s(dest, sizeof(uint8_t), src + *tracker, 1);
    *tracker += 1;
}

uint32_t* load_png_file(FILE* file, int* result_width, int* result_height)
{
    show_message(L"start");
    *result_width = 0;
    *result_height = 0;
    if (!file) { return NULL; }

    rewind(file);

    const int signature_len = 8;
    uint8_t signature[signature_len];
    if (fread(signature, 1, signature_len, file) < signature_len) { return NULL; }
    
    const uint8_t png_signature[] = {137, 80, 78, 71, 13, 10, 26, 10};
    for (int i = 0; i < signature_len; i++)
    {
        if (signature[i] != png_signature[i]) { return NULL; }
    }

    uint32_t palettes[256];

    int compressed_tracker = 0;
    int compressed_len = 1024;
    uint8_t* compressed_data = malloc(compressed_len);
    if (!compressed_data) { return NULL; }

    uint32_t width = 0;
    uint32_t height = 0;
    uint8_t bit_depth = 0;
    uint8_t colour_type = 0;
    uint8_t interlacing = 0;

    while (true)
    {
        uint32_t chunk_len = 0;
        if (fread(&chunk_len, 4, 1, file) < 1) { return NULL; }
        chunk_len = reverse_bytes(chunk_len);

        uint8_t chunk_type[4];
        if (fread(&chunk_type, 1, 4, file) < 1) { return NULL; }

        show_message(L"%c%c%c%c", chunk_type[0], chunk_type[1], chunk_type[2], chunk_type[3]);

        uint8_t* chunk_data = NULL;
        if (chunk_len > 0)
        {
            chunk_data = malloc(chunk_len);
            if (!chunk_data) { return NULL; }
            if (fread(chunk_data, 1, chunk_len, file) < chunk_len) { free(chunk_data); return NULL; }
        }

        int ancillary = (chunk_type[0] & 0x20) >> 4;
        //private, reserved, safe-to-copy chunk properties not checked

        if (memcmp(chunk_type, "IHDR", 4) == 0)
        {
            int addr = 0;
            read_word(&width, chunk_data, &addr);
            read_word(&height, chunk_data, &addr);
            read_byte(&bit_depth, chunk_data, &addr);
            read_byte(&colour_type, chunk_data, &addr);
            addr += 2; //skip compression and filter
            read_byte(&interlacing, chunk_data, &addr);
        }
        else if (memcmp(chunk_type, "PLTE", 4) == 0)
        {
            int palette_tracker = 0;
            int data_tracker = 0;
            while (data_tracker < chunk_len)
            {
                read_word(palettes + palette_tracker, chunk_data, &data_tracker);
                data_tracker -= 1; //only advance through data by 3, not 4
                palette_tracker++; //next palette
                if (palette_tracker >= 256) { free(compressed_data); return NULL; }
            }
        }
        else if (memcmp(chunk_type, "IDAT", 4) == 0)
        {
            while (compressed_len < compressed_tracker + chunk_len)
            {
                if (compressed_len > INT_MAX / 2) { free(compressed_data); return NULL; }
                compressed_len *= 2;

                uint8_t* temp = realloc(compressed_data, compressed_len);
                if (!temp) { free(compressed_data); return NULL; }
                compressed_data = temp;
            }

            memcpy_s(compressed_data + compressed_tracker, compressed_len - compressed_tracker, chunk_data, chunk_len);
            compressed_tracker += chunk_len;
        }
        else if (memcmp(chunk_type, "IEND", 4) != 0 && !ancillary) { free(chunk_data); return NULL; } //unsupported critical chunk

        uint32_t chunk_crc = 0;
        if (fread(&chunk_crc, 4, 1, file) < 1) { free(chunk_data); return NULL; }
        chunk_crc = reverse_bytes(chunk_crc);

        uint8_t* crc_buffer = malloc(4 + chunk_len);
        if (!crc_buffer) { free(chunk_data); return NULL; }

        memcpy_s(crc_buffer, 4 + chunk_len, (void*)chunk_type, 4);
        if (chunk_data != NULL) { memcpy_s(crc_buffer + 4, chunk_len, (void*)chunk_data, chunk_len); }
        if (!valid_crc(crc_buffer, 4 + chunk_len, chunk_crc)) { free(chunk_data); return NULL; }

        free(crc_buffer);
        free(chunk_data);
        if (memcmp(chunk_type, "IEND", 4) == 0) { break; }
    }

    free(compressed_data);
    return NULL;

    show_message(L"a");
    //todo - support indexed colour type properly
    int channel_count = 0;
    switch (colour_type)
    {
        case GRAYSCALE: channel_count = 1; break;
        case TRUECOLOUR: channel_count = 3; break;
        case INDEXED_PALETTE: channel_count = 1; break;
        case GRAYSCALE_ALPHA: channel_count = 2; break;
        case TRUECOLOUR_ALPHA: channel_count = 4; break;
        default: return NULL;
    }

    show_message(L"b");
    const int bit_pixel_size = channel_count * bit_depth;
    const int bpp = (7 + bit_pixel_size)/8;
    if (bpp > 4) { free(compressed_data); return NULL; } //image too high quality for me to implement smh

    show_message(L"c");
    const int row_size = (7 + width * bit_pixel_size)/8;
    uint8_t* scanlined_pixels = malloc(height * row_size * sizeof(uint8_t));

    show_message(L"d");
    const int filtered_row_size = 1 + row_size;
    const int filtered_pixels_size = height * filtered_row_size;
    uint8_t* filtered_pixels = malloc(filtered_pixels_size * sizeof(uint8_t));
    show_message(L"time to uncompress");
    if (uncompress(filtered_pixels, (long unsigned int*)&filtered_pixels_size,
    compressed_data, (long unsigned int)compressed_tracker) != Z_OK) { free(compressed_data); return NULL; }
    free(compressed_data);
    show_message(L"uncompressed");

    for (int row = 0; row < height; row++)
    {
        const uint8_t filter_type = filtered_pixels[row * filtered_row_size];
        for (int col = 1; col <= width; col++) //starting after filter type byte
        {
            show_message(L"filtering with %d", filter_type);

            const int result_index = row * row_size + col - 1;
            const uint8_t curr = filtered_pixels[row * filtered_row_size + col];
            if (filter_type == FILTER_NONE) { scanlined_pixels[result_index] = curr % 256; continue; }

            uint8_t left = 0;
            if (col >= 1 + bpp) { left = filtered_pixels[row * filtered_row_size + col - bpp]; }
            if (filter_type == FILTER_SUB) { scanlined_pixels[result_index] = (curr + left) % 256; continue; }

            uint8_t up = 0;
            if (row > 0) { up = filtered_pixels[(row - 1) * filtered_row_size + col]; }
            if (filter_type == FILTER_UP) { scanlined_pixels[result_index] = (curr + up) % 256; continue; }
            if (filter_type == FILTER_AVERAGE) { scanlined_pixels[result_index] = (curr + (up + left)/2) % 256; continue; }

            uint8_t up_left = 0;
            if (col >= 1 + bpp && row > 0) { up_left = filtered_pixels[(row - 1) * filtered_row_size + col - bpp]; }
            if (filter_type == FILTER_PAETH) { scanlined_pixels[result_index] = (curr + paeth(left, up, up_left)) % 256; continue; }

            free(filtered_pixels);
            free(scanlined_pixels);
            return NULL;
        }
    }

    free(filtered_pixels);

    uint32_t* long_scanlined_pixels = malloc(width * height);

    show_message(L"time to convert");
    //Conversion to AGRB (reverse BGRA)
    for (int i = 0; i < width * height; i++)
    {
        const int start_bit = i * bit_pixel_size;

        for (int bit = 0; bit < bit_pixel_size; bit++)
        {
            const int curr_byte = (start_bit + bit) / 8;
            const int curr_bit = (start_bit + bit) % 8;
            const int offset = 7 - curr_bit;
            long_scanlined_pixels[i] <<= 1;
            long_scanlined_pixels[i] += (scanlined_pixels[curr_byte] >> offset) & 1;
        }
    }

    free(scanlined_pixels);

    if (interlacing == INTERLACING_NONE)
    {
        if (result_width) { *result_width = width; }
        if (result_height) { *result_height = height; }
        return long_scanlined_pixels;
    }

    //todo - adam7 interlacing
    
    free(long_scanlined_pixels);
    return NULL; //temp
}