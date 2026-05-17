#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <zlib.h>
#include <math.h>
#include "./file_loader.h"
#include "./debug.h"

typedef unsigned long ulong_t;
#define SIGNATURE_LEN 8

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

    for (uint16_t i = 0; i < 256; i++)
    {
        uint32_t crc_output = i;
        for (uint8_t j = 0; j < 8; j++)
        {
            //Losing a bit after shift
            if (crc_output & 1) { crc_output = (crc_output >> 1) ^ polynomial_coefficients; }
            else { crc_output >>= 1; }
        }

        crc_table[i] = crc_output;
    }

    generated_crc = true;
}

static bool valid_crc(uint8_t* buff, ulong_t len, uint32_t crc)
{
    uint32_t crc_output = 0xFFFFFFFF;
    if (!generated_crc) { generate_crc_table(); }

    for (ulong_t i = 0; i < len; i++) { crc_output = crc_table[ (crc_output ^ buff[i]) & 0xFF ] ^ (crc_output >> 8); }
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

static uint32_t convert_sizes(uint32_t num, uint8_t from_bit_size, uint8_t to_bit_size)
{
    const uint8_t from_max = pow(2, from_bit_size) - 1;
    const uint8_t to_max = pow(2, to_bit_size) - 1;
    num %= from_max + 1;
    int result = (num * to_max) / (from_max);
    result %= to_max + 1;
    return result;
}

static void read_word(uint32_t* dest, uint8_t* src, int* tracker)
{
    memcpy_s(dest, sizeof(uint32_t), src + *tracker, 4);
    *dest = reverse_bytes(*dest);
    *tracker += 4;
}

static void read_rgb(uint32_t* dest, uint8_t* src, int* tracker)
{
    memcpy_s(dest, sizeof(uint32_t), src + *tracker, 3);
    *dest = reverse_bytes(*dest);
    *dest |= 0xC0;
    *tracker += 3;
}

static void read_byte(uint8_t* dest, uint8_t* src, int* tracker)
{
    memcpy_s(dest, sizeof(uint8_t), src + *tracker, 1);
    *tracker += 1;
}

uint32_t* load_png_file(FILE* file, int* result_width, int* result_height)
{
    uint8_t* compressed_data = NULL;
    uint8_t* chunk_data = NULL;
    uint8_t* crc_buffer = NULL;
    uint8_t* filtered_pixels = NULL;
    uint8_t* packed_pixels = NULL;
    uint32_t* interlaced_pixels = NULL;

    *result_width = 0;
    *result_height = 0;
    if (!file) { goto fail; }

    rewind(file);

    ulong_t compressed_tracker = 0;
    ulong_t compressed_len = 1024;
    compressed_data =  calloc(compressed_len, sizeof(uint8_t));
    if (!compressed_data) { goto fail; }

    uint8_t signature[SIGNATURE_LEN];
    if (fread(signature, 1, SIGNATURE_LEN, file) < SIGNATURE_LEN) { goto fail; }
    
    const uint8_t png_signature[] = {137, 80, 78, 71, 13, 10, 26, 10};
    for (uint8_t i = 0; i < SIGNATURE_LEN; i++)
    {
        if (signature[i] != png_signature[i]) { goto fail; }
    }

    uint32_t palettes[256];
    
    if (!compressed_data) { goto fail; }

    uint32_t width = 0;
    uint32_t height = 0;
    uint8_t bit_depth = 0;
    uint8_t colour_type = 0;
    uint8_t interlacing = 0;

    while (true)
    {
        uint32_t chunk_len = 0;
        if (fread(&chunk_len, 4, 1, file) < 1) { goto fail; }
        chunk_len = reverse_bytes(chunk_len);

        crc_buffer = calloc(4 + chunk_len, sizeof(uint8_t));
        if (!crc_buffer) { goto fail; }

        uint8_t chunk_type[4];
        if (fread(&chunk_type, 1, 4, file) < 1) { goto fail; }
        memcpy_s(crc_buffer, 4 + chunk_len, chunk_type, 4);

        chunk_data = NULL;
        if (chunk_len > 0)
        {
            chunk_data = calloc(chunk_len, sizeof(uint8_t));
            if (!chunk_data || fread(chunk_data, 1, chunk_len, file) < chunk_len) { goto fail; }

            memcpy_s(crc_buffer + 4, chunk_len, chunk_data, chunk_len);
        }

        bool ancillary = (chunk_type[0] & 0x20) >> 5;
        //private, reserved, safe-to-copy chunk properties not checked

        if (memcmp(chunk_type, "IHDR", 4) == 0)
        {
            uint8_t addr = 0;
            read_word(&width, chunk_data, &addr);
            read_word(&height, chunk_data, &addr);
            read_byte(&bit_depth, chunk_data, &addr);
            read_byte(&colour_type, chunk_data, &addr);
            addr += 2; //skip compression and filter
            read_byte(&interlacing, chunk_data, &addr);
        }
        else if (memcmp(chunk_type, "PLTE", 4) == 0)
        {
            uint16_t palette_tracker = 0;
            uint16_t data_tracker = 0;
            while (data_tracker < chunk_len)
            {
                read_rgb(palettes + palette_tracker, chunk_data, &data_tracker);
                palette_tracker++; //next palette
                if (palette_tracker >= 256) { goto fail; }
            }
        }
        else if (memcmp(chunk_type, "IDAT", 4) == 0)
        {
            while (compressed_len < compressed_tracker + chunk_len)
            {
                if (compressed_len > INT_MAX / 2) { goto fail; }
                compressed_len *= 2;

                uint8_t* temp = realloc(compressed_data, compressed_len);
                if (!temp) { goto fail; }
                compressed_data = temp;
            }

            memcpy_s(compressed_data + compressed_tracker, compressed_len - compressed_tracker, chunk_data, chunk_len);
            compressed_tracker += chunk_len;
        }
        else if (memcmp(chunk_type, "IEND", 4) != 0 && !ancillary) { goto fail; } //unsupported critical chunk

        uint32_t chunk_crc = 0;
        if (fread(&chunk_crc, 4, 1, file) < 1) { goto fail; }
        chunk_crc = reverse_bytes(chunk_crc);

        if (!valid_crc(crc_buffer, 4 + chunk_len, chunk_crc)) { goto fail; }

        free(crc_buffer);
        free(chunk_data);
        crc_buffer = NULL;
        chunk_data = NULL;
        if (memcmp(chunk_type, "IEND", 4) == 0) { break; }
    }

    //todo - support indexed colour type properly
    uint8_t channel_count = 0;
    switch (colour_type)
    {
        case GRAYSCALE: channel_count = 1; break;
        case TRUECOLOUR: channel_count = 3; break;
        case INDEXED_PALETTE: channel_count = 1; break;
        case GRAYSCALE_ALPHA: channel_count = 2; break;
        case TRUECOLOUR_ALPHA: channel_count = 4; break;
        default: return NULL;
    }

    const uint8_t bits_per_pixel = channel_count * bit_depth;
    const uint8_t bytes_per_pixel = (7 + bits_per_pixel) / 8;

    const uint32_t filtered_packed_width = 1 + (7 + width * bits_per_pixel) / 8;
    const ulong_t filtered_packed_size = filtered_packed_width * height;
    const uint32_t packed_width = filtered_packed_width - 1;

    filtered_pixels = calloc(filtered_packed_size, sizeof(uint8_t));
    packed_pixels = calloc(packed_width * height, sizeof(uint8_t));
    interlaced_pixels = calloc(width * height, sizeof(uint32_t));

    if (!filtered_pixels || !packed_pixels || !interlaced_pixels ) { goto fail; }
    if (bits_per_pixel > 32) { goto fail; } //image too high quality for me to implement smh

    if (uncompress(filtered_pixels, &filtered_packed_size, compressed_data, compressed_tracker) != Z_OK) { goto fail; }
    free(compressed_data);
    compressed_data = NULL;
    show_message(L"uncompressed");

    //todo - interlacing support

    for (uint32_t row = 0; row < height; row++)
    {
        const uint8_t filter_type = filtered_pixels[filtered_packed_width * row];
        for (uint32_t col = 1; col < filtered_packed_width; col++) //starting after filter type byte
        {
            const ulong_t result_index = packed_width * row + col - 1;
            const uint8_t curr = filtered_pixels[filtered_packed_width * row + col];

            uint8_t left = 0;
            if (col > bytes_per_pixel) { left = filtered_pixels[filtered_packed_width * row + col - bytes_per_pixel]; }

            uint8_t up = 0;
            if (row > 0) { up = filtered_pixels[filtered_packed_width * (row - 1) + col]; }

            uint8_t up_left = 0;
            if (col > bytes_per_pixel && row > 0) { up_left = filtered_pixels[filtered_packed_width * (row - 1) + col - bytes_per_pixel]; }

            switch (filter_type)
            {
                case FILTER_NONE:
                packed_pixels[result_index] = curr % 256;
                break;

                case FILTER_SUB:
                packed_pixels[result_index] = (curr + left) % 256;
                break;

                case FILTER_UP:
                packed_pixels[result_index] = (curr + up) % 256;
                break;

                case FILTER_AVERAGE:
                packed_pixels[result_index] = (curr + (up + left)/2) % 256;
                break;

                case FILTER_PAETH:
                packed_pixels[result_index] = result_byte = (curr + paeth(left, up, up_left)) % 256;
                break;

                default:
                goto fail;
            }
        }
    }

    //note for later, unpacked pixels stores in big endian
    show_message(L"time to unpack");
    for (uint32_t row = 0; row < height; row++)
    {
        interlaced_pixels[row * width] = packed_pixels[row * packed_width];
        for (uint32_t col = 0; col < width; col++)
        {
            const ulong_t start_packed_bit = row * packed_width * 8 + col * bits_per_pixel + 8;

            for (uint8_t bit = 0; bit < bits_per_pixel; bit++)
            {
                const ulong_t curr_unpacked_byte = row * width + col * bytes_per_pixel + (bit / 8) + 1;
                const ulong_t curr_packed_byte = (start_packed_bit + bit) / 8;
                const uint8_t curr_packed_bit = (start_packed_bit + bit) % 8;
                const uint8_t offset = 7 - curr_packed_bit;
                interlaced_pixels[curr_unpacked_byte] <<= 1;
                interlaced_pixels[curr_unpacked_byte] += (packed_pixels[curr_packed_byte] >> offset) & 1;

                if (bits_per_pixel < 8 && bit == bits_per_pixel - 1)
                {
                    interlaced_pixels[curr_unpacked_byte] = convert_sizes(interlaced_pixels[curr_unpacked_byte], bits_per_pixel, 8);
                }
            }
        }
    }

    free(packed_pixels);
    packed_pixels = NULL;

    

    free(filtered_pixels);
    filtered_pixels = NULL;

    if (interlacing == INTERLACING_ADAM)
    {
        //todo - adam7 interlacing
    }

    for (int32_t pixel = 0; pixel < width * height; pixel++)
    {

    }

    show_message(L"DONE!");
    if (result_width) { *result_width = width; }
    if (result_height) { *result_height = height; }
    return packed_pixels;

    fail:
    free(interlaced_pixels);
    free(filtered_pixels);
    free(packed_pixels);
    free(crc_buffer);
    free(compressed_data);
    free(chunk_data);
    interlaced_pixels = NULL;
    filtered_pixels = NULL;
    packed_pixels = NULL;
    crc_buffer = NULL;
    compressed_data = NULL;
    chunk_data = NULL;
    return NULL;
}