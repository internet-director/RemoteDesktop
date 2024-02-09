#include <iostream>
#include <string>
#include <thread>
#include <format>
#include <fstream>
#include <vector>
#include <memory>

#include "stdafx.h"
#include "config.h"
#include "Compressor.h"
#include "WSACleaner.h"
#include "Client.h"

#pragma comment (lib,"ws2_32.lib")
#pragma comment (lib,"Gdiplus.lib")

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
    std::vector<BYTE> data, data_compressed;

    if (!client.init(12345, "127.0.0.1"))
    {
        return 1;
    }

    if (!compressor.init_workspace())
    {
        return 1;
    }


    RECT rect;

    HWND hwndDesk = GetDesktopWindow();
    if (GetWindowRect(hwndDesk, &rect) == FALSE)
    {
        return 1;
    }

    HDC hdcDesk = GetDC(hwndDesk);
    HDC hdcScreen = CreateCompatibleDC(hdcDesk);
    HBITMAP bitmap = CreateCompatibleBitmap(hdcDesk, rect.right, rect.bottom);
    SelectObject(hdcScreen, bitmap);


    bool send_status = false;
    
    while (true)
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

            BitBlt(hdcScreen, 0, 0, frame.width, frame.height, hdcDesk, 0, 0, SRCCOPY);
            GetDIBits(hdcDesk, bitmap, 0, 0, NULL, &bi, DIB_RGB_COLORS);

            frame.size = ((bi.bmiHeader.biWidth * bi.bmiHeader.biBitCount + 31) & ~31) / 8 * bi.bmiHeader.biHeight;

            data.resize(frame.size);
            data_compressed.resize(frame.size);
            GetDIBits(hdcDesk, bitmap, 0, bi.bmiHeader.biHeight, data.data(), (BITMAPINFO*)&bi, DIB_RGB_COLORS);

            frame.c_size =
                compressor.compress(data.data(), frame.size, data_compressed.data(), frame.size);

            if (frame.c_size == -1)
            {
                continue;
            }
        }

        send_status = client.send(&frame, sizeof(frame));
        send_status = client.send(data_compressed.data(), frame.c_size);
        Sleep(30);
    }

    DeleteObject(bitmap);
    DeleteDC(hdcScreen);
    ReleaseDC(hwndDesk, hdcDesk);

    return 0;
}