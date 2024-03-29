#include "Client.h"
#include "Compressor.h"
#include "WSACleaner.h"
#include "config.h"
#include "stdafx.h"
#include <condition_variable>
#include <unordered_set>

#include <atomic>
#include <format>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Gdiplus.lib")

int monitorNumber = 0;

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
	MONITORINFOEX monitorInfo = { 0 };
	monitorInfo.cbSize = sizeof(MONITORINFOEX);
	GetMonitorInfoW(hMonitor, &monitorInfo);

	if (dwData != NULL)
	{
		reinterpret_cast< std::vector< MONITORINFOEX >* >(dwData)->emplace_back(std::move(monitorInfo));
	}
	return TRUE;
}

void update_monitors(std::vector< MONITORINFOEX >& result)
{
	EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, reinterpret_cast< LPARAM >(&result));
}

int main()
{
	WSACleaner wsa;

	if (!wsa.inited())
	{
		std::cerr << "WSA initialisation error: " << wsa.getErrorCode() << std::endl;
		return 1;
	}

	Client client;
	WSADATA wsaData = { 0 };
	Compressor compressor;
	std::vector< BYTE > data, data_compressed;

	if (!client.init(12345, "127.0.0.1"))
	{
		return 1;
	}

	if (!compressor.init_workspace())
	{
		return 1;
	}

	RECT rect;
	CLIENT_CONFIGURATION configuration{};

	HWND hwndDesk = GetDesktopWindow();
	if (GetWindowRect(hwndDesk, &rect) == FALSE)
	{
		return 1;
	}

	HDC hdcDesk = GetDC(hwndDesk);
	HDC hdcScreen = CreateCompatibleDC(hdcDesk);

	std::atomic_bool send_kill{ false };

	bool send_status = false;
	while (!send_kill)
	{
		if (!send_status)
		{
			client.close();

			if (!client.try_connect())
			{
				std::cerr << "cant connect\n";
				Sleep(1000);
				continue;
			}
		}

		FRAME_INFO frame = { 0 };

		{
			BITMAPINFO bi = { 0 };

			if (GetWindowRect(hwndDesk, &rect) == FALSE)
			{
				continue;
			}

			frame.width = rect.right - rect.left;
			frame.height = rect.bottom - rect.top;
			bi.bmiHeader.biCompression = BI_RGB;
			bi.bmiHeader.biSize = sizeof(bi.bmiHeader);

			HBITMAP bitmap = CreateCompatibleBitmap(hdcDesk, frame.width, frame.height);
			SelectObject(hdcScreen, bitmap);
			BitBlt(hdcScreen, 0, 0, frame.width, frame.height, hdcDesk, 0, 0, SRCCOPY);
			GetDIBits(hdcDesk, bitmap, 0, 0, NULL, &bi, DIB_RGB_COLORS);

			frame.size = ((bi.bmiHeader.biWidth * bi.bmiHeader.biBitCount + 31) & ~31) / 8 * bi.bmiHeader.biHeight;

			data.resize(frame.size);
			data_compressed.resize(frame.size);
			GetDIBits(hdcDesk, bitmap, 0, bi.bmiHeader.biHeight, data.data(), (BITMAPINFO*)&bi, DIB_RGB_COLORS);
			DeleteObject(bitmap);

			frame.c_size = compressor.compress(data.data(), frame.size, data_compressed.data(), frame.size);

			if (frame.c_size == -1)
			{
				continue;
			}
		}

		std::vector< MONITORINFOEX > monitors;
		update_monitors(monitors);

		configuration = { .monitorCount = (int)monitors.size() };

		send_status = client.send(&configuration, sizeof(CLIENT_CONFIGURATION));

		if (!send_status)
		{
			
		}

		for (auto& it : monitors)
		{
			send_status = client.send(&it, sizeof(MONITORINFOEX));
		}

		send_status = client.send(&frame, sizeof(frame));
		send_status = client.send(data_compressed.data(), frame.c_size);
		Sleep(30);
	}

	send_kill.store(false);
	DeleteDC(hdcScreen);
	ReleaseDC(hwndDesk, hdcDesk);

	return 0;
}