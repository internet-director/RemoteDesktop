#pragma once
#include "../client/stdafx.h"

LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

ATOM W_Register(WNDPROC lpfnWndProc);

HWND W_Create(PRECT rect);
