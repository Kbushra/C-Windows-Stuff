#pragma once
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <zlib.h>
#include <math.h>
#include "./debug.h"

extern uint32_t* load_png_file(FILE* file, int* result_width, int* result_height);