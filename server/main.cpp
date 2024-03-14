#include "../client/Compressor.h"
#include "../client/WSACleaner.h"
#include "../client/config.h"
#include "../client/stdafx.h"
#include "Server.h"
#include "Window.h"

#include <format>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
#include <stop_token>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

std::wstring draw_monitors(const MONITORINFOEX& monitorInfo)
{
	std::wstringstream result;
	result << L"Monitor Name: " << monitorInfo.szDevice << std::endl;
	result << L"Monitor Dimensions: " << monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left << L"x"
		   << monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top << std::endl;
	result << L"Monitor Position: (" << monitorInfo.rcMonitor.left << L", " << monitorInfo.rcMonitor.top << L")" << std::endl;
	result << L"-------------------------------------------" << std::endl << std::endl;
	return result.str();
}

void drawer(const std::atomic_bool& recv_kill, Server& server, HDC hdcWin)
{
	CLIENT_CONFIGURATION client_metadata;
	Socket client;
	RECT rect = { 0 };
	Compressor compressor;
	std::vector< char > data, data_decompressed;
	std::vector< MONITORINFOEX > monitors;

	if (!compressor.inited())
	{
		return;
	}

	HFONT hFont = CreateFontW(
		14, 
		0, 
		0, 
		0, 
		FW_BOLD, 
		FALSE, 
		FALSE, 
		FALSE, 
		DEFAULT_CHARSET, 
		OUT_OUTLINE_PRECIS, 
		CLIP_DEFAULT_PRECIS, 
		ANTIALIASED_QUALITY, 
		VARIABLE_PITCH, 
		L"Arial"
	);

	SelectObject(hdcWin, hFont);

	while (!recv_kill)
	{
		ULONG d_size;
		FRAME_INFO frame;
		
		if (!client.recv(&client_metadata, sizeof(client_metadata)))
		{
			server.accept(client);
			continue;
		}

		monitors.resize(client_metadata.monitorCount);

		for (int i = 0; i < client_metadata.monitorCount; i++)
		{
			MONITORINFOEX monitorInfo{ 0 };
			client.recv(&monitorInfo, sizeof(MONITORINFOEX));
			monitors[i] = monitorInfo;
		}

		if (!client.recv(&frame, sizeof(frame)))
		{
			server.accept(client);
			continue;
		}

		data.resize(frame.c_size);

		if (!client.recv(data.data(), frame.c_size))
		{
			server.accept(client);
			continue;
		}

		data_decompressed.resize(frame.size);

		if ((d_size = compressor.decompress(data.data(), frame.c_size, data_decompressed.data(), frame.size)) == -1)
		{
			continue;
		}

		{
			rect.right = frame.width;
			rect.bottom = frame.height;

			BITMAPINFO bmpInfo;
			bmpInfo.bmiHeader.biSize = sizeof(bmpInfo.bmiHeader);
			bmpInfo.bmiHeader.biPlanes = 1;
			bmpInfo.bmiHeader.biBitCount = 32;
			bmpInfo.bmiHeader.biCompression = BI_RGB;
			bmpInfo.bmiHeader.biClrUsed = 0;

			bmpInfo.bmiHeader.biWidth = frame.width;
			bmpInfo.bmiHeader.biHeight = frame.height;
			bmpInfo.bmiHeader.biSizeImage = frame.size;

			HBITMAP bitmap = CreateCompatibleBitmap(hdcWin, rect.right, rect.bottom);
			SetDIBits(hdcWin, bitmap, 0, rect.bottom, data_decompressed.data(), &bmpInfo, DIB_RGB_COLORS);

			RECT textRect{ 0 };
			auto txt = draw_monitors(monitors[0]);

			HBRUSH brush = CreatePatternBrush(bitmap);
			FillRect(hdcWin, &rect, brush);
			SetTextColor(hdcWin, RGB(255, 255, 255));
			//SetBkMode(hdcWin, TRANSPARENT);
			SetBkColor(hdcWin, RGB(0, 0, 0));

			textRect.left = 10;	
			textRect.top = 10;	
			textRect.right = 1000;
			textRect.bottom = 500;

			DrawTextW(hdcWin, txt.c_str(), txt.length(), &textRect, DT_LEFT | DT_TOP | DT_NOCLIP);
			DeleteObject(brush);
			DeleteObject(bitmap);
		}
	}

	DeleteObject(hFont);
}

// TODO: support for window resizing

int main()
{
	WSACleaner wsa;

	if (!wsa.inited())
	{
		std::cerr << "WSA initialisation error: " << wsa.getErrorCode() << std::endl;
		return 1;
	}

	Server server;

	if (!server.init(12345))
	{
		return 1;
	}

	RECT rect;
	HWND hwndDesk = GetDesktopWindow();
	CLIENT_CONFIGURATION configuration{ .monitorCount = 1};
	SERVER_CONFIGURATION conf{ .size = 50 };
	CLIENT_CONFIGURATION new_configuration{ 0 };

	if (GetWindowRect(hwndDesk, &rect) == FALSE)
	{
		return 1;
	}

	auto tmp = rect;
	tmp.right /= (100 / conf.size);
	tmp.bottom /= (100 / conf.size);

	W_Register(WindowProc, &configuration);

	HWND win = W_Create(&tmp);
	HDC hdcWin = GetDC(win);

	std::atomic_bool recv_kill = false;

	std::thread recv_thread(
		drawer, 
		std::cref(recv_kill), 
		std::ref(server),
		std::ref(hdcWin)
	);

	MSG msg;

	while (GetMessageW(&msg, win, 0, 0) > 0)
	{
		if (new_configuration.monitorCount != configuration.monitorCount)
		{
			new_configuration = configuration;
			CallWindowProcW(WindowProc, win, WM_CREATE, 0, reinterpret_cast< LPARAM >(&configuration));
		}

		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	recv_kill.store(true);
	recv_thread.join();

	if (win)
		CloseWindow(win);

	ReleaseDC(win, hdcWin);

	return 0;
}