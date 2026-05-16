#ifndef UNICODE
#define UNICODE
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "./headers/debug.h"
#include "./headers/file_loader.h"
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

#define SPRITE_COUNT 4
sprite_object** test = NULL;

int game_loop(HWND window)
{
    if (!test)
    {
        test = malloc(4 * sizeof(sprite_object*));
        FILE* file = fopen("./image.png", "rb");
        if (!file) { return 404; }

        for (int i = 0; i < SPRITE_COUNT; i++)
        {
            test[i] = malloc(sizeof(sprite_object));
            if (!test[i]) { return 507; }
            
            *(test[i]) = default_sprite_object;
            test[i]->x = i * 60;
            test[i]->y = i * 40;
            test[i]->sprite_pixels = load_png_file(file, &(test[i]->sprite_width), &(test[i]->sprite_height));
        }

        fclose(file);
    }
    
    PostMessage(window, WM_PAINT, 0, 0);
    return 200;
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
        if (errcode >= 300) { PostMessage(window, WM_QUIT, 0, 0); }

        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (msg.message == WM_QUIT)
        {
            if (errcode >= 300) { show_message(L"error: %d", errcode); }
            return errcode;
        }

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
            for (int i = 0; i < SPRITE_COUNT; i++)
            {
                free(test[i]);
                test[i] = NULL;
            }
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

            RECT client;
            GetClientRect(window, &client);
            FillRect(display, &client, dynamic_brush(RGB(0, 0, 0), &main_brush));

            if (test != NULL)
            {
                for (int i = 0; i < SPRITE_COUNT; i++)
                {
                    draw_pixels(display, test[i]->sprite_pixels, test[i]->x, test[i]->y, test[i]->sprite_width, test[i]->sprite_height);
                }
            }

			EndPaint(window, &painter);
		return 0;
	}

	return DefWindowProc(window, message, message_arg1, message_arg2);
}