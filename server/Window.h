#pragma once
#include "../client/config.h"
#include "../client/stdafx.h"

LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

ATOM W_Register(WNDPROC lpfnWndProc, PCLIENT_CONFIGURATION pclient);

HWND W_Create(PRECT rect);
