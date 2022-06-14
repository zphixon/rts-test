#include <sstream>
#include <Windows.h>

void ErrorExit(wchar_t const* lpszFunction, HRESULT hr) {
    // Retrieve the system error message for the last-error code

    LPWSTR lpMsgBuf;

    FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        hr,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR) &lpMsgBuf,
        0, NULL);

	std::wstringstream ss;
	ss << "" << lpszFunction << L" failed:\r\n" << "0x" << std::hex << hr << std::dec << L" " << lpMsgBuf;
	std::wstring str = ss.str();

    MessageBoxW(NULL, (LPCWSTR)str.c_str(), L"Error", MB_OK); 

    LocalFree(lpMsgBuf);
    ExitProcess(hr); 
}

void ErrorExit(wchar_t const* lpszFunction) {
    ErrorExit(lpszFunction, GetLastError());
}