#include "Window.h"

#include <string>

#define CLASS_NAME L"CLASS_NAME"

LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	PCLIENT_CONFIGURATION configuration = reinterpret_cast< PCLIENT_CONFIGURATION >(lParam);

	switch (msg)
	{
	case WM_CREATE:
	{
		HMENU hMenubar = CreateMenu();
		HMENU hQualityMenu = CreateMenu();

		AppendMenuA(hMenubar, MF_POPUP, (UINT_PTR)hQualityMenu, "Desktop");

		if (configuration != nullptr)
		{
			for (int i = 0; i < configuration->monitorCount; i++)
			{
				AppendMenuA(hQualityMenu, MF_STRING, i, std::to_string(i + 1).c_str());
			}
		}

		SetMenu(hWnd, hMenubar);
		break;
	};
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		break;
	}
	default:
	{
		return DefWindowProcW(hWnd, msg, wParam, lParam);
	}
	};
	return 0;
}

ATOM W_Register(WNDPROC lpfnWndProc)
{
	WNDCLASSEXW cls = { 0 };
	cls.cbSize = sizeof(WNDCLASSEX);
	cls.style = CS_HREDRAW | CS_VREDRAW;
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

	HWND win =
		CreateWindowExW(NULL, CLASS_NAME, title, WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, rect->right, rect->bottom, NULL, NULL, GetModuleHandleW(NULL), NULL);

	if (win == NULL)
	{
		return NULL;
	}

	ShowWindow(win, SW_SHOW);
	UpdateWindow(win);
	return win;
}
