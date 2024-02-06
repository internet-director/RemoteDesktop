#include <iostream>
#include <vector>
#include <thread>
#include <fstream>
#include <format>

#include "../client/stdafx.h"
#include "../client/Compressor.h"
#include "../client/WSACleaner.h"
#include "Server.h"

#define CLASS_NAME L"CLASS_NAME"
#pragma comment(lib, "ws2_32.lib")

LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
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

ATOM W_Register(WNDPROC lpfnWndProc) {
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

HWND W_Create(PRECT rect) {
    PWCHAR title = (PWCHAR)L"TITLE";

    HWND win = CreateWindowExW(NULL,
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
        NULL);

    if (win == NULL) {
        return NULL;
    }

    ShowWindow(win, SW_SHOW);
    UpdateWindow(win);
    return win;
}

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

    HWND win        = W_Create(&tmp);
    HDC hdcWin      = GetDC(win);
    HDC hdcDesk     = GetDC(hwndDesk);
    HDC cdcScreen   = CreateCompatibleDC(hdcDesk);
    HBITMAP bitmap  = CreateCompatibleBitmap(hdcDesk, rect.right, rect.bottom);
    SelectObject(cdcScreen, bitmap);


    std::atomic_bool recv_kill = false;
    std::vector<char> data, data_decompressed;

    std::thread recv_thread(
        [&]
        {
        while (!recv_kill) {
            ULONG sz;
            ULONG c_size;
            size_t cnt = 0;
            int bytesReceived;

            bytesReceived = client.recv(&sz, sizeof(sz));

            if (bytesReceived == -1) break;
            data.resize(sz);

            do {
                bytesReceived = client.recv(data.data() + cnt, sz - cnt);

                if (bytesReceived == -1) break;
                
                cnt += bytesReceived;
                std::cout << cnt << "\n";
            } while (cnt != sz);

            {
                size_t sz = ((rect.right * 32 + 31) & ~31) / 8 * rect.bottom;

                data_decompressed.resize(sz);

                if ((c_size = compressor.decompress(data.data(), cnt, data_decompressed.data(), sz)) == -1)
                {
                    continue;
                }
            }


            if (cnt != 0) {
                BITMAPINFO bmpInfo;
                bmpInfo.bmiHeader.biSize = sizeof(bmpInfo.bmiHeader);
                bmpInfo.bmiHeader.biPlanes = 1;
                bmpInfo.bmiHeader.biBitCount = 32;
                bmpInfo.bmiHeader.biCompression = BI_RGB;
                bmpInfo.bmiHeader.biClrUsed = 0;

                bmpInfo.bmiHeader.biWidth = rect.right;
                bmpInfo.bmiHeader.biHeight = rect.bottom;
                bmpInfo.bmiHeader.biSizeImage = 
                    ((bmpInfo.bmiHeader.biWidth * bmpInfo.bmiHeader.biBitCount + 31) & ~31) / 8 * bmpInfo.bmiHeader.biHeight;


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