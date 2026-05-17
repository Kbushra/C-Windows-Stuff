#include <windows.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <wchar.h>

extern int show_bytes(uint8_t* bytes, int len);
extern int v_show_message(wchar_t* message, va_list args);
extern int show_message(wchar_t* message, ...);