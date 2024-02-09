#include <iostream>
#include <vector>
#include <thread>
#include <fstream>
#include <format>

#include "../client/stdafx.h"
#include "../client/Compressor.h"
#include "../client/WSACleaner.h"
#include "../client/config.h"
#include "Server.h"
#include "Window.h"

#pragma comment(lib, "ws2_32.lib")



//TODO: support for window resizing

int main()
{
    WSACleaner wsa;

    if (!wsa.inited())
    {
        std::cerr << "WSA initialisation error: " << wsa.getErrorCode() << std::endl;
        return 1;
    }

    Compressor compressor;
    Socket client;
    Server server;

    if (!compressor.inited())
    {
        return 1;
    }

    if (!server.init(12345) || !server.accept(client))
    {
        return 1;
    }


    RECT rect;
    HWND hwndDesk = GetDesktopWindow();

    if (GetWindowRect(hwndDesk, &rect) == FALSE)
    {
        return 1;
    }

    auto tmp = rect;
    tmp.right /= 1.5;
    tmp.bottom /= 1.5;

    W_Register(WindowProc);

    HDC hdcDesk     = GetDC(hwndDesk);
    HDC cdcScreen   = CreateCompatibleDC(hdcDesk);
    HWND win        = W_Create(&tmp);
    HDC hdcWin      = GetDC(win);
    HBITMAP bitmap  = CreateCompatibleBitmap(hdcDesk, rect.right, rect.bottom);
    SelectObject(cdcScreen, bitmap);


    std::atomic_bool recv_kill = false;
    std::vector<char> data, data_decompressed;

    std::thread recv_thread(
        [&]
        {
            while (!recv_kill) {
                ULONG d_size;
                FRAME_INFO frame;

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
                    rect = { 0 };

                    rect.right  = frame.width;
                    rect.bottom = frame.height;

                    std::cerr << rect.right << " " << rect.bottom << "\n";

                    BITMAPINFO bmpInfo;
                    bmpInfo.bmiHeader.biSize = sizeof(bmpInfo.bmiHeader);
                    bmpInfo.bmiHeader.biPlanes = 1;
                    bmpInfo.bmiHeader.biBitCount = 32;
                    bmpInfo.bmiHeader.biCompression = BI_RGB;
                    bmpInfo.bmiHeader.biClrUsed = 0;

                    bmpInfo.bmiHeader.biWidth = frame.width;
                    bmpInfo.bmiHeader.biHeight = frame.height;
                    bmpInfo.bmiHeader.biSizeImage = frame.size;

                    SetDIBits(hdcWin, bitmap, 0, rect.bottom,
                        data_decompressed.data(), &bmpInfo, DIB_RGB_COLORS);

                    HBRUSH brush = CreatePatternBrush(bitmap);
                    FillRect(hdcWin, &rect, brush);
                    DeleteObject(brush);
                }
            }
        });

    MSG msg;

    while (GetMessageW(&msg, win, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    recv_kill.store(true);
    recv_thread.join();

clean:
    if (win) CloseWindow(win);

    ReleaseDC(win, hdcWin);
    DeleteDC(cdcScreen);
    DeleteObject(bitmap);
    ReleaseDC(NULL, hdcDesk);

    return 0;
}