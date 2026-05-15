#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "./file_loader.h"

static uint32_t crc_table[256];
static bool generated_crc = false;

static void generate_crc_table(void)
{
    const uint32_t polynomial_coefficients = 0x04C11DB7;

    for (int i = 0; i < 256; i++)
    {
        uint32_t crc_output = i << 24;
        for (int j = 0; j < 8; j++)
        {
            //Losing a bit after shift
            if (crc_output & 0x80000000) { crc_output = (crc_output << 1) ^ polynomial_coefficients; }
            else { crc_output <<= 1; }
        }

        crc_table[i] = crc_output;
    }

    generated_crc = true;
}

static bool valid_crc(uint8_t* buff, int len, uint32_t crc)
{
    uint32_t crc_output = 0xFFFFFFFF;
    if (!generated_crc) { generate_crc_table(); }

    for (int i = 0; i < len; i++) { crc_output = crc_table[ ((crc_output >> 24) ^ buff[i]) & 0xFF ] ^ (crc_output << 8); }
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

static uint32_t type_to_uint32(char* type)
{
    if (!type) { return 0; }

    uint32_t result = 0;
    for (int i = 0; i < 4; i++)
    {
        if (*type == 0) { return result; }
        result <<= 8;
        result += *type;
        type++;
    }

    return result;
}

static void read_word(uint32_t* dest, uint8_t* src, int* tracker)
{
    memcpy(dest, src + *tracker, 4);
    *dest = reverse_bytes(*dest);
    *tracker += 4;
}

static void read_byte(uint8_t* dest, uint8_t* src, int* tracker)
{
    memcpy(dest, src + *tracker, 1);
    *tracker += 1;
}

uint32_t* load_png_file(FILE* file, int* result_width, int* result_height)
{
    if (!file) { return NULL; }

    const int signature_len = 8;
    uint8_t signature[signature_len];
    if (fread(signature, 1, signature_len, file) < signature_len) { return NULL; }
    
    const uint8_t png_signature[] = {137, 80, 78, 71, 13, 10, 26, 10};
    for (int i = 0; i < signature_len; i++)
    {
        if (signature[i] != png_signature[i]) { return NULL; }
    }

    const uint32_t idat = type_to_uint32("IDAT");
    const uint32_t ihdr = type_to_uint32("IHDR");
    const uint32_t iend = type_to_uint32("IEND");
    const uint8_t* compressed_pixels = NULL;

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

        uint32_t chunk_type = 0;
        if (fread(&chunk_type, 4, 1, file) < 1) { return NULL; }
        chunk_type = reverse_bytes(chunk_type);

        uint8_t* chunk_data = malloc(chunk_len);
        if (fread(chunk_data, 1, chunk_len, file) < chunk_len) { return NULL; }
        //ancillary, private, reserved, safe-to-copy chunk properties not checked

        if (chunk_type == ihdr)
        {
            int addr = 0;
            read_word(&width, chunk_data, &addr);
            read_word(&height, chunk_data, &addr);
            read_byte(&bit_depth, chunk_data, &addr);
            read_byte(&colour_type, chunk_data, &addr);
            addr += 2; //skip compression and filter
            read_byte(&interlacing, chunk_data, &addr);

            compressed_pixels = malloc(width * height);
        }

        uint32_t chunk_crc = 0;
        if (fread(&chunk_crc, 4, 1, file) < 1) { return NULL; }

        uint8_t* crc_buffer = malloc(4 + chunk_len);
        memcpy(crc_buffer, (void*)&chunk_type, 4);
        memcpy(crc_buffer + 4, (void*)chunk_data, chunk_len);
        if (!valid_crc(crc_buffer, 4 + chunk_len, chunk_crc)) { return NULL; }

        free(crc_buffer);
        free(chunk_data);
        if (chunk_type == iend) { break; }
    }

    //debug
    *result_width = 1000;
    *result_height = 1000;
    uint32_t* pixels = malloc(*result_width * *result_height * sizeof(uint32_t));
    for (int i = 0; i < *result_width; i++)
    { for (int j = 0; j < *result_height; j++) { pixels[i * (*result_height)+j] = 0xFFFFFFFF; } }
    return pixels;

    if (result_width) { *result_width = width; }
    if (result_height) { *result_height = height; }
    return NULL; //temp
}