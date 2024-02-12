#include <Windows.h>
#include <atomic>
#include <format>
#include <iostream>
#include <thread>

void SaveBitmapToFile(HDC hdc, HBITMAP hBitmap, const char* filename)
{
	BITMAP bmp;
	BITMAPINFO bi = { 0 };
	BITMAPFILEHEADER bmfHeader;
	DWORD dwDIBSize;
	HANDLE hFile;
	DWORD dwBytesWritten;

	bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
	GetDIBits(hdc, hBitmap, 0, 0, NULL, &bi, DIB_RGB_COLORS);

	hFile = CreateFileA(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		std::cerr << "Failed to create file." << std::endl;
		return;
	}

	dwDIBSize = ((bi.bmiHeader.biWidth * bi.bmiHeader.biBitCount + 31) & ~31) / 8 * bi.bmiHeader.biHeight;
	// dwDIBSize = bi.bmiHeader.biWidth * bi.bmiHeader.biHeight * 3;

	bmfHeader.bfType = 0x4D42;	  // "BM"
	bmfHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwDIBSize;
	bmfHeader.bfReserved1 = 0;
	bmfHeader.bfReserved2 = 0;
	bmfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	bi.bmiHeader.biCompression = BI_RGB;

	WriteFile(hFile, &bmfHeader, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
	WriteFile(hFile, &bi.bmiHeader, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);

	BYTE* lpBits = new BYTE[dwDIBSize];

	if (!lpBits)
	{
		std::cerr << "Failed to allocate memory." << std::endl;
		CloseHandle(hFile);
		return;
	}

	GetDIBits(hdc, hBitmap, 0, bi.bmiHeader.biHeight, lpBits, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

	WriteFile(hFile, lpBits, dwDIBSize, &dwBytesWritten, NULL);

	delete[] lpBits;
	CloseHandle(hFile);
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		break;
	}
	case WM_NCHITTEST:
	{
		return HTCAPTION;
	}
	case WM_PAINT:
	{
	}
	default:
	{
		return DefWindowProcW(hWnd, msg, wParam, lParam);
	}
	};
	return 0;
}

#define CLASS_NAME L"CLASS_NAME"

ATOM W_Register(WNDPROC lpfnWndProc, HINSTANCE hInstance)
{
	WNDCLASSEXW cls = { 0 };
	cls.cbSize = sizeof(WNDCLASSEX);
	cls.style = CS_DBLCLKS;
	cls.lpfnWndProc = lpfnWndProc;
	cls.hInstance = GetModuleHandleW(NULL);
	cls.hIcon = LoadIconW(NULL, IDI_APPLICATION);
	cls.hCursor = LoadCursorW(NULL, IDC_ARROW);
	cls.hbrBackground = (HBRUSH)COLOR_WINDOW;
	cls.lpszClassName = CLASS_NAME;
	cls.hIconSm = LoadIconW(NULL, IDI_APPLICATION);
	return RegisterClassExW(&cls);
}

HWND W_Create(PRECT rect)
{
	PWCHAR title = (PWCHAR)L"TITLE";

	HWND win = CreateWindowExW(
		WS_EX_TOPMOST,
		CLASS_NAME,
		title,
		WS_OVERLAPPEDWINDOW | WS_THICKFRAME,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		rect->right,
		rect->bottom,
		NULL,
		NULL,
		GetModuleHandleW(NULL),
		NULL);

	if (win == NULL)
	{
		return NULL;
	}

	ShowWindow(win, SW_SHOW);
	UpdateWindow(win);
	return win;
}

void LoadScreen(HWND hWnd, HBITMAP bitmap, HDC hdcScreen)
{
	RECT rect;
	HDC hdc = GetDC(hWnd);
	HDC hdcMem = CreateCompatibleDC(hdc);
	BITMAP bm;

	GetWindowRect(hWnd, &rect);
	GetObjectW(bitmap, sizeof(BITMAP), &bm);

	int w = rect.right - rect.left;
	int h = rect.bottom - rect.top;
	HBITMAP hTempBitmap = CreateCompatibleBitmap(hdc, w, h);
	SelectObject(hdcMem, hTempBitmap);

	StretchBlt(hdcMem, 0, 0, w, h, hdcScreen, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);

	HBRUSH brush = CreatePatternBrush(hTempBitmap);
	rect.left = rect.top = 0;
	rect.right = w;
	rect.bottom = h;
	FillRect(hdc, &rect, brush);

	DeleteObject(brush);
	DeleteObject(hTempBitmap);
	DeleteDC(hdcMem);
	ReleaseDC(hWnd, hdc);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	WNDCLASS wc = {};

	int code = 0;
	HDC hdcDesk;
	RECT rect;
	HWND hwndDesk;
	HWND win = NULL;
	HWINSTA station = NULL;
	LPCWSTR desktopName = L"test";
	HDESK desktop = NULL;
	HDC hdcScreen;
	HBITMAP bitmap;
	HGDIOBJ hOld;
	BITMAPINFO bitInfo;
	RECT tmp;
	std::atomic_bool worker = true;

	std::thread th_msg;

	if ((desktop = CreateDesktopW(desktopName, NULL, NULL, 0, GENERIC_ALL, NULL)) == NULL)
	{
		code = 1;
		goto clean;
	}

	/*if (SetThreadDesktop(desktop) == FALSE)
	{
		code = 1;
		goto clean;
	}*/

	hwndDesk = GetDesktopWindow();
	if (GetWindowRect(hwndDesk, &rect) == FALSE)
	{
		code = 1;
		goto clean;
	}

	hdcDesk = GetDC(hwndDesk);
	hdcScreen = CreateCompatibleDC(hdcDesk);
	bitmap = CreateCompatibleBitmap(hdcDesk, rect.right, rect.bottom);
	hOld = SelectObject(hdcScreen, bitmap);

	tmp = rect;
	tmp.right /= 1.5;
	tmp.bottom /= 1.5;

	W_Register(WindowProc, hInstance);
	win = W_Create(&tmp);

	if (win == NULL)
	{
		code = 1;
		goto clean;
	}

	th_msg = std::thread(
		[&hdcScreen, &rect, &hdcDesk, &win, &bitmap, &worker]
		{
			while (worker)
			{
				BitBlt(hdcScreen, 0, 0, rect.right, rect.bottom, hdcDesk, 0, 0, SRCCOPY);
				// LoadScreen(win, bitmap, hdcDesk);
				SaveBitmapToFile(hdcDesk, bitmap, std::format("F:\\tmp\\{}_{}.bmp", time(0), clock()).c_str());
				Sleep(100);
			}
		});

	MSG msg;

	while (GetMessageW(&msg, win, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	worker.store(false);
	th_msg.join();

	DeleteObject(bitmap);
	ReleaseDC(NULL, hdcDesk);
	DeleteDC(hdcScreen);

clean:
	if (win)
		CloseWindow(win);
	if (station)
		CloseWindowStation(station);
	if (desktop)
		CloseDesktop(desktop);

	return code;
}