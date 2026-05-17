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

LONG WINAPI crash_handler(EXCEPTION_POINTERS* info)
{
    FILE* report = fopen("./report.log", "w");
    fprintf(report, "Encountered exception code %08lX\n", info->ExceptionRecord->ExceptionCode);
    fprintf(report, "At address %p\n", info->ExceptionRecord->ExceptionAddress);
    fprintf(report, "Flags: %08lX\n", info->ExceptionRecord->ExceptionFlags);
    fclose(report);
    return EXCEPTION_EXECUTE_HANDLER;
}

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
        test = calloc(SPRITE_COUNT, sizeof(sprite_object*));
        if (!test) { return 507; }

        for (int i = 0; i < SPRITE_COUNT; i++)
        {
            test[i] = malloc(sizeof(sprite_object));
            if (!test[i]) { return 507; }
            
            *(test[i]) = default_sprite_object;
            test[i]->x = i * 60;
            test[i]->y = i * 40;
        }

        FILE* file = fopen("./images/main.png", "rb");
        if (!file) { return 404; }
        test[0]->sprite_pixels = load_png_file(file, &(test[0]->sprite_width), &(test[0]->sprite_height));
        fclose(file);
    }
    
    PostMessage(window, WM_PAINT, 0, 0);
    return 200;
}

LRESULT CALLBACK WindowProc(HWND window, UINT action, WPARAM action_arg1, LPARAM action_arg2);
int WINAPI wWinMain(HINSTANCE program, HINSTANCE _, PWSTR args, int window_state)
{
    SetUnhandledExceptionFilter(crash_handler);

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
        if (errcode >= 300) { PostMessage(window, WM_QUIT, errcode, 0); }

        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (msg.message == WM_QUIT)
        {
            return errcode;
        }

        DWORD elapsed_time = GetTickCount() - prev_time;
        if (elapsed_time < FRAME_TIME) { Sleep(FRAME_TIME - elapsed_time); }
    }
}

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM message_arg1, LPARAM message_arg2)
{
    HDC display;
    PAINTSTRUCT painter;
    RECT client;

	switch (message)
	{
		case WM_DESTROY:
            for (int i = 0; i < SPRITE_COUNT; i++)
            {
                free(test[i]);
                test[i] = NULL;
            }
            free(test);
            test = NULL;
            DeleteObject(main_brush);

            //Error code
            if (message_arg1 < 300) { message_arg1 = 0; }
			PostQuitMessage(message_arg1);
		return 0;

        case WM_SETCURSOR:
            if (LOWORD(message_arg2) == HTCLIENT)
            {
                SetCursor(LoadCursorW(NULL, IDC_ARROW));
                return TRUE;
            }
        break;

		case WM_PAINT:
			display = BeginPaint(window, &painter);

            GetClientRect(window, &client);
            FillRect(display, &client, dynamic_brush(RGB(0, 0, 0), &main_brush));

            if (test)
            {
                for (int i = 0; i < SPRITE_COUNT; i++)
                {
                    if (!test[i]) { continue; }
                    draw_pixels(display, test[i]->sprite_pixels, test[i]->x, test[i]->y, test[i]->sprite_width, test[i]->sprite_height);
                }
            }

			EndPaint(window, &painter);
		return 0;
	}

	return DefWindowProc(window, message, message_arg1, message_arg2);
}