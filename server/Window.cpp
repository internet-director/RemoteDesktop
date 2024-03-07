#include "Window.h"

#include <string>

PCLIENT_CONFIGURATION configuration = nullptr;

#define CLASS_NAME L"CLASS_NAME"
#define SIZE_50  1000
#define SIZE_75  1001
#define SIZE_100 1002

HMENU hMenubar{ NULL };
HMENU hSizeMenu{ NULL };
HMENU hQualityMenu{ NULL };

void free_hmenu()
{
	if (hQualityMenu != NULL)
		DestroyMenu(hQualityMenu);
	if (hSizeMenu != NULL)
		DestroyMenu(hSizeMenu);
	if (hMenubar != NULL)
		DestroyMenu(hMenubar);

	hMenubar	 = { NULL };
	hSizeMenu	 = { NULL };
	hQualityMenu = { NULL };
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CREATE:
	{
		free_hmenu();
		hMenubar = CreateMenu();
		hSizeMenu = CreateMenu();
		hQualityMenu = CreateMenu();

		AppendMenuA(hMenubar, MF_POPUP, (UINT_PTR)hSizeMenu, "Size");
		AppendMenuA(hMenubar, MF_POPUP, (UINT_PTR)hQualityMenu, "Desktop");

		int i = 0;
		for (; i < configuration->monitorCount; i++)
		{
			AppendMenuA(hQualityMenu, MF_STRING, i, std::to_string(i + 1).c_str());
		}

		AppendMenuA(hSizeMenu, MF_STRING, SIZE_50, "50%");
		AppendMenuA(hSizeMenu, MF_STRING, SIZE_75, "75%");
		AppendMenuA(hSizeMenu, MF_STRING, SIZE_100, "100%");

		SetMenu(hWnd, hMenubar);
		break;
	};
	case WM_COMMAND:
	{
		int wmID = LOWORD(wParam);
		switch (wmID)
		{
		case SIZE_50:
			//configuration->size = 50;
			break;
		case SIZE_100:
			//configuration->size = 100;
			break;
		default:
			return DefWindowProcW(hWnd, msg, wParam, lParam);
		}
		break;
	}
	case WM_DESTROY:
	{
		free_hmenu();
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

ATOM W_Register(WNDPROC lpfnWndProc, PCLIENT_CONFIGURATION pclient)
{
	configuration = pclient;
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
	PWCHAR title = (PWCHAR)L"Half-Life 3";

	HWND win = CreateWindowExW(
			NULL, 
			CLASS_NAME, 
			title, 
			WS_OVERLAPPEDWINDOW | WS_VISIBLE, 
			CW_USEDEFAULT, 
			CW_USEDEFAULT, 
			rect->right, 
			rect->bottom, 
			NULL, 
			NULL, 
			GetModuleHandleW(NULL), 
			NULL
		);

	if (win == NULL)
	{
		return NULL;
	}

	ShowWindow(win, SW_SHOW);
	UpdateWindow(win);
	return win;
}
