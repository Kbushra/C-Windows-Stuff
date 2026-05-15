#ifndef UNICODE
#define UNICODE
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "./headers/assets/room_object.h"

#define HANDLE_ERROR 0

#define FPS 60
#define FRAME_TIME 1000.0/FPS

#define WINDOW_WIDTH 960
#define WINDOW_HEIGHT 720

HBRUSH main_brush = NULL;
HBRUSH dynamic_brush(COLORREF col, HBRUSH* brush)
{
    if (*brush)
    {
        LOGBRUSH log_brush;
        GetObject(*brush, sizeof(log_brush), &log_brush);
        if (log_brush.lbColor == col) { return *brush; }

        DeleteObject(*brush);
    }
    
    *brush = CreateSolidBrush(col);
    return *brush;
}

sprite_object* test = NULL;

int game_loop(HWND window)
{
    if (!test)
    {
        MessageBoxW(NULL, L"creating sprite", L"debug", MB_OK);
        test = malloc(sizeof(sprite_object));
        *test = default_sprite_object;
        FILE* file = fopen("./image.png", "rb");
        if (!file) { return 404; }

        MessageBoxW(NULL, L"loading png", L"debug", MB_OK);
        test->sprite_pixels = load_png_file(file, &(test->sprite_width), &(test->sprite_height));
        MessageBoxW(NULL, L"SUCCESSFUL LOAD!", L"debug", MB_OK);

        if (test->sprite_pixels != NULL)
        {
            wchar_t buff[128];
            swprintf(buff, 128, L"pixel1 = %u", test->sprite_pixels[0]);
            MessageBoxW(NULL, buff, L"debug", MB_OK);
        }
        else { MessageBoxW(NULL, L"rip", L"debug", MB_OK); }
    }
    
    draw_pixels(GetDC(window), test->sprite_pixels, test->x, test->y, test->sprite_width, test->sprite_height);
    return 0;
}

LRESULT CALLBACK WindowProc(HWND window, UINT action, WPARAM action_arg1, LPARAM action_arg2);
int WINAPI wWinMain(HINSTANCE program, HINSTANCE _, PWSTR args, int window_state)
{
	const wchar_t class_name[] = L"Class";
	WNDCLASS window_class = {};
	window_class.lpfnWndProc = WindowProc;
	window_class.lpszClassName = class_name;
	window_class.hInstance = program;
	window_class.hIcon = LoadImageW(NULL, L"./icon.ico", IMAGE_ICON, 100, 100, LR_LOADFROMFILE);
	RegisterClass(&window_class);

	HWND window = CreateWindowW(class_name, L"Image Renderer", WS_OVERLAPPEDWINDOW, 0, 0,
        WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, program, NULL);

	if (window == HANDLE_ERROR) { return 0; }

	ShowWindow(window, window_state);

	MSG msg = {};

    while (TRUE)
    {
        DWORD prev_time = GetTickCount();
        int errcode = game_loop(window);
        if (errcode >= 300) { return errcode; }

        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (msg.message == WM_QUIT) { break; }

        DWORD elapsed_time = GetTickCount() - prev_time;
        if (elapsed_time < FRAME_TIME) { Sleep(FRAME_TIME - elapsed_time); }
    }
}

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM message_arg1, LPARAM message_arg2)
{
    HDC display;

	switch (message)
	{
		case WM_DESTROY:
            free(test);
            DeleteObject(main_brush);
			PostQuitMessage(0);
		return 0;

        case WM_SETCURSOR:
            if (LOWORD(message_arg2) == HTCLIENT)
            {
                SetCursor(LoadCursorW(NULL, IDC_ARROW));
                return TRUE;
            }
        break;

		case WM_PAINT:
            PAINTSTRUCT painter;
			display = BeginPaint(window, &painter);
            FillRect(display, &painter.rcPaint, dynamic_brush(RGB(0, 0, 0), &main_brush));
			EndPaint(window, &painter);
		return 0;
	}

	return DefWindowProc(window, message, message_arg1, message_arg2);
}