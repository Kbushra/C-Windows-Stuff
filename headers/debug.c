#include <windows.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <wchar.h>

int show_bytes(uint8_t* bytes, int len)
{
    int buff_len = 1;
    for (int i = 0; i < len; i++)
    {
        int advance = _scwprintf(L"%02X ", bytes[i]);
        if (advance < 0) { return 0; }
        buff_len += advance;
    }

    wchar_t* buff = calloc(buff_len, sizeof(wchar_t)); 
    if (!buff) { return 0; }

    int tracker = 0;
    for (int i = 0; i < len; i++)
    {
        int advance = swprintf(buff + tracker, buff_len - tracker, L"%02X ", bytes[i]);
        if (advance < 0) { free(buff); return 0; }
        tracker += advance;
    }

    int out = MessageBoxW(NULL, buff, L"debug", MB_OK);
    free(buff);
    return out;
}

int v_show_message(const wchar_t* message, va_list args)
{
    if (!wcschr(message, L'%')) { return MessageBoxW(NULL, message, L"debug", MB_OK); }

    va_list counting_args;
    va_copy(counting_args, args);
    const int buff_len = _vscwprintf(message, counting_args) + 1;
    va_end(counting_args);

    if (buff_len <= 0) { return 0; }

    wchar_t* buff = calloc(buff_len, sizeof(wchar_t)); 
    if (!buff) { return 0; }
    
    if (vswprintf(buff, buff_len, message, args) < 0) { free(buff); return 0; }

    int out = MessageBoxW(NULL, buff, L"debug", MB_OK);
    free(buff);
    return out;
}

int show_message(const wchar_t* message, ...)
{
    if (!wcschr(message, L'%')) { return MessageBoxW(NULL, message, L"debug", MB_OK); }

    va_list args;
    va_start(args, message);
    int result = v_show_message(message, args);
    va_end(args);

    return result;
}