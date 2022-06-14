#include <iostream>
#include <wchar.h>
#include <windows.h>
#include <winuser.h>
#include <hidusage.h>

#include "MyRtsPlugin.h"
#include "errorexit.h"

MyRtsPlugin* rtsPlugin = NULL;
HBRUSH green = CreateSolidBrush(RGB(166, 255, 190));
HBRUSH red = CreateSolidBrush(RGB(255, 166, 166));

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
	if (rtsPlugin != NULL) {
		std::cout << "x=" << rtsPlugin->x << " y=" << rtsPlugin->y
			<< " p=" << rtsPlugin->pressure << " s=" << rtsPlugin->buttonStatus << std::endl;
	}

	switch (msg) {
	case WM_DESTROY: {
		PostQuitMessage(0);
		return 0;
	}

	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		if (rtsPlugin != NULL && rtsPlugin->buttonStatus == MyRtsPlugin::TouchStatus::Inverted) {
			FillRect(hdc, &ps.rcPaint, red);
		}
		else {
			FillRect(hdc, &ps.rcPaint, green);
		}

		EndPaint(hwnd, &ps);
		return 0;
	}

	case WM_KEYDOWN: {
		if (wp == VK_ESCAPE) {
			PostQuitMessage(0);
			return 0;
		}
	}
	}

	return DefWindowProcW(hwnd, msg, wp, lp);
}

int WINAPI WinMain(
	HINSTANCE instance,
	HINSTANCE,
	LPSTR cmdLine,
	int nCmd
) {
	// attach console and redirect stdin/stdout/stderr because when we build
	// with /SUBSYSTEM:WINDOWS we don't get a console window
	if (!AllocConsole()) ErrorExit(L"AllocConsole");

	FILE* f = NULL; // this is gross
	freopen_s(&f, "CONIN$", "w", stdin);
	freopen_s(&f, "CONOUT$", "w", stderr);
	freopen_s(&f, "CONOUT$", "w", stdout);

	const wchar_t className[] = L"my window";

	WNDCLASS wc = {};
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = instance;
	wc.lpszClassName = className;
	RegisterClass(&wc);

	HWND hwnd = CreateWindowExW(
		0,
		className,
		L"my window title",
		WS_OVERLAPPEDWINDOW, 
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL,
		NULL,
		instance,
		NULL
	);

	if (hwnd == NULL) ErrorExit(L"CreateWindowExW");

	rtsPlugin = new MyRtsPlugin(hwnd);

	ShowWindow(hwnd, nCmd);

	MSG msg = {};
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	rtsPlugin->Release();

	return 0;
}
