#pragma once
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <winsock.h>
#include <stdlib.h>

extern uint32_t* read_png_file(FILE* file, int* result_width, int* result_height);