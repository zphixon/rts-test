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
	switch (uMsg) {
	case WM_DESTROY: {
		PostQuitMessage(0);
		return 0;
	}

	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		if (reh->isInverted) {
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

LPCSTR deviceTypeStr(DWORD type) {
	switch (type) {
	case RIM_TYPEHID:
		return "hid";
	case RIM_TYPEKEYBOARD:
		return "kb";
	case RIM_TYPEMOUSE:
		return "mouse";
	default:
		return "unknown";
	}
}

void enumerateRawDevices() {
	UINT nDevices = 0;
	UINT succ = GetRawInputDeviceList(NULL, &nDevices, sizeof(RAWINPUTDEVICELIST));
	if (succ == (UINT)-1) {
		ErrorExit(L"get num devices");
	}

	PRAWINPUTDEVICELIST deviceList = (PRAWINPUTDEVICELIST)calloc(nDevices, sizeof(RAWINPUTDEVICELIST));
	if (deviceList == NULL) {
		ErrorExit(L"alloc device list");
	}

	succ = GetRawInputDeviceList(deviceList, &nDevices, sizeof(RAWINPUTDEVICELIST));
	if (succ == (UINT)-1) {
		ErrorExit(L"get devices");
	}
	std::cout << "got " << nDevices << " devices" << std::endl;

	for (UINT i = 0; i < succ; i++) {
		UINT size = 0;
		UINT succ = GetRawInputDeviceInfo(deviceList[i].hDevice, RIDI_DEVICENAME, NULL, &size);
		if (succ != 0) {
			ErrorExit(L"device name string length");
		}

		LPWSTR name = (LPWSTR)calloc(size, sizeof(WCHAR));
		if (name == NULL) {
			ErrorExit(L"alloc device name");
		}

		succ = GetRawInputDeviceInfo(deviceList[i].hDevice, RIDI_DEVICENAME, (LPVOID)name, &size);
		if (succ <= 0) {
			ErrorExit(L"device name");
		}

		std::cout << deviceTypeStr(deviceList[i].dwType) << " ";
		std::wcout << std::wstring(name) << std::endl;
	}
}

int WINAPI WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR pCmdLine,
	int nCmdShow
) {
	//
	// attach console
	//
	if (!AllocConsole()) {
		ErrorExit(L"alloc console");
	}

	FILE* f = NULL; // this is gross
	freopen_s(&f, "CONIN$", "w", stdin);
	freopen_s(&f, "CONOUT$", "w", stderr);
	freopen_s(&f, "CONOUT$", "w", stdout);

	enumerateRawDevices();

	// 
	// register for raw input messages
	//

	RAWINPUTDEVICE rid[2];
	// mouse
	rid[0].usUsagePage = 0x01;
	rid[0].usUsage = 0x02;
	rid[0].dwFlags = 0;
	rid[0].hwndTarget = 0;
	// keyboard
	rid[1].usUsagePage = 0x01;
	rid[1].usUsage = 0x06;
	rid[1].dwFlags = 0;
	rid[1].hwndTarget = 0;
	if (RegisterRawInputDevices(rid, 2, sizeof(RAWINPUTDEVICE)) == FALSE) {
		ErrorExit(L"register devices");
	}

	//
	// get the information of registered devices
	//

	UINT nDevices = 0;
	UINT succ = GetRegisteredRawInputDevices(NULL, &nDevices, sizeof(RAWINPUTDEVICE));
	if (succ == (UINT)-1) {
		ErrorExit(L"get num reg devices");
	}

	std::cout << succ << " registered devices" << std::endl;

	const wchar_t CLASS_NAME[] = L"my window";

	WNDCLASS wc = {};
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;
	RegisterClass(&wc);

	HWND hwnd = CreateWindowExW(
		0,
		CLASS_NAME,
		L"my window title uwu",
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
