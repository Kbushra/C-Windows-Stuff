#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "./file_loader.h"
//#include "./debug.h"

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
    uint8_t* compressed_pixels = NULL;
    int pixel_tracker = 0;

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

        uint8_t* chunk_data;
        if (chunk_len > 0)
        {
            chunk_data = malloc(chunk_len);
            if (!chunk_data) { return NULL; }
            if (fread(chunk_data, 1, chunk_len, file) < chunk_len) { free(chunk_data); return NULL; }
        }
        else { chunk_data = NULL; }

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

            compressed_pixels = malloc(width * height);
            if (!compressed_pixels) { free(chunk_data); return NULL; }
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
            }
        }
        else if (memcmp(chunk_type, "IDAT", 4) == 0)
        {
            memcpy_s(compressed_pixels + pixel_tracker, width * height - pixel_tracker, chunk_data, chunk_len);
            pixel_tracker += chunk_len;
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

    //debug
    *result_width = 100;
    *result_height = 80;
    uint32_t* pixels = malloc(*result_width * *result_height * sizeof(uint32_t));
    if (!pixels) { return NULL; }
    for (int y = 0; y < *result_height; y++)
    {
        for (int x = 0; x < *result_width; x++) { pixels[y * (*result_width)+x] = 0xFFFFFFFF; }
    }
    return pixels;

    if (result_width) { *result_width = width; }
    if (result_height) { *result_height = height; }
    return NULL; //temp
}