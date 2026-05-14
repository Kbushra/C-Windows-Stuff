#ifndef UNICODE
#define UNICODE
#endif

#include "./headers/assets/assets.h"

#define HANDLE_ERROR 0

#define FPS 60
#define FRAME_TIME 1000.0/FPS

#define WINDOW_WIDTH 960
#define WINDOW_HEIGHT 720
#define WM_DRAW (WM_USER)

uint32_t pixels[WINDOW_WIDTH * WINDOW_HEIGHT/2];

HBRUSH main_brush = NULL;
HBRUSH dynamic_brush(COLORREF col, HBRUSH* brush)
{
    if (*brush != NULL)
    {
        LOGBRUSH log_brush;
        GetObject(*brush, sizeof(log_brush), &log_brush);
        if (log_brush.lbColor == col) { return *brush; }

        DeleteObject(*brush);
    }
    
    *brush = CreateSolidBrush(col);
    return *brush;
}

int game_loop(HWND window)
{
    SendMessage(window, WM_DRAW, 0, 0);
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
        game_loop(window);

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
            
        case WM_DRAW:
            display = GetDC(window);

            for (int i = 0; i < WINDOW_WIDTH * WINDOW_HEIGHT/2; i++)
            {
                pixels[i] = 0xFFFFFFFF;
            }

            draw_pixels(display, pixels, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT/2);
            ReleaseDC(window, display);
        return 0;
	}

	return DefWindowProc(window, message, message_arg1, message_arg2);
}