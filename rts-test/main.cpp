#include <iostream>
#include <wchar.h>
#include <windows.h>
#include <winuser.h>
#include <hidusage.h>

#include "RtsEventHandler.h"
#include "errorexit.h"

RtsEventHandler* reh = NULL;
HBRUSH green;
HBRUSH red;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (reh != NULL) {
		std::cout << "x=" << reh->x << " y=" << reh->y << " p=" << reh->pressure << " s=" << reh->buttonStatus << std::endl;
	}

	switch (uMsg) {
	case WM_DESTROY: {
		PostQuitMessage(0);
		return 0;
	}

	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		if (reh->buttonStatus == RtsEventHandler::TouchStatus::Inverted) {
			FillRect(hdc, &ps.rcPaint, red);
		}
		else {
			FillRect(hdc, &ps.rcPaint, green);
		}

		EndPaint(hwnd, &ps);
		return 0;
	}

	case WM_KEYDOWN: {
		if (wParam == VK_ESCAPE) {
			PostQuitMessage(0);
			return 0;
		}
	}
	}

	return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR pCmdLine,
	int nCmdShow
) {
	// attach console and redirect stdin/stdout/stderr because when we build
	// with /SUBSYSTEM:WINDOWS we don't get a console window
	if (!AllocConsole()) {
		ErrorExit(L"alloc console");
	}

	FILE* f = NULL; // this is gross
	freopen_s(&f, "CONIN$", "w", stdin);
	freopen_s(&f, "CONOUT$", "w", stderr);
	freopen_s(&f, "CONOUT$", "w", stdout);

	const wchar_t CLASS_NAME[] = L"my window";

	WNDCLASS wc = {};
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;
	RegisterClass(&wc);

	HWND hwnd = CreateWindowExW(
		0,
		CLASS_NAME,
		L"my window title",
		WS_OVERLAPPEDWINDOW, 
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	if (hwnd == NULL) {
		return 1;
	}

	reh = new RtsEventHandler(hwnd);
	red = CreateSolidBrush(RGB(255, 166, 166));
	green = CreateSolidBrush(RGB(166, 255, 190));

	ShowWindow(hwnd, nCmdShow);

	MSG msg = {};
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	reh->Release();

	return 0;
}
