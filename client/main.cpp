#include <iostream>
#include <string>
#include <thread>
#include <format>
#include <fstream>
#include <vector>
#include <memory>

#include "stdafx.h"
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



    if (!client.init(12345, "192.168.0.4"))
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

        uint32_t dwDIBSize = 0;

        {
            BITMAPINFO bi = { 0 };

            if (GetWindowRect(hwndDesk, &rect) == FALSE)
            {
                continue;
            }

            BitBlt(hdcScreen, 0, 0, rect.right, rect.bottom, hdcDesk, 0, 0, SRCCOPY);

            bi.bmiHeader.biCompression = BI_RGB;
            bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
            GetDIBits(hdcDesk, bitmap, 0, 0, NULL, &bi, DIB_RGB_COLORS);

            dwDIBSize = ((bi.bmiHeader.biWidth * bi.bmiHeader.biBitCount + 31) & ~31) / 8 * bi.bmiHeader.biHeight;

            data.resize(dwDIBSize);
            data_compressed.resize(dwDIBSize);

            GetDIBits(hdcDesk, bitmap, 0, bi.bmiHeader.biHeight, data.data(), (BITMAPINFO*)&bi, DIB_RGB_COLORS);
        }

        {
            dwDIBSize =
                compressor.compress(data.data(), dwDIBSize, data_compressed.data(), dwDIBSize);

            if (dwDIBSize == -1)
            {
                continue;
            }

            send_status = client.send(&dwDIBSize, sizeof(dwDIBSize));
            send_status = client.send(data_compressed.data(), dwDIBSize);
        }

        Sleep(30);
    }

    DeleteObject(bitmap);
    DeleteDC(hdcScreen);
    ReleaseDC(hwndDesk, hdcDesk);

    return 0;
}