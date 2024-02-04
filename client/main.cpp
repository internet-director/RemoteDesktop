#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
#include <string>
#include <WinSock2.h>
#include <Windows.h>
#include <thread>
#include <format>
#include <fstream>
#include <vector>
#include <memory>
#include <gdiplus.h>
#include <GdiPlusFlat.h>

#include "Client.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment (lib,"Gdiplus.lib")

typedef NTSTATUS(__stdcall* fpRtlDecompressBuffer)(USHORT, PUCHAR, ULONG, PUCHAR, ULONG, PULONG);
typedef NTSTATUS(__stdcall* fpRtlCompressBuffer)(USHORT, PUCHAR, ULONG, PUCHAR, ULONG, ULONG, PULONG, PVOID);
typedef NTSTATUS(__stdcall* fpRtlGetCompressionWorkSpaceSize)(USHORT, PULONG, PULONG);


fpRtlCompressBuffer fRtlCompressBuffer;
fpRtlDecompressBuffer fRtlDecompressBuffer;
fpRtlGetCompressionWorkSpaceSize fRtlGetCompressionWorkSpaceSize;

int main()
{
    HMODULE hNTDLL = GetModuleHandleA("ntdll.dll");
    if (hNTDLL == NULL) {
        std::cerr << "Unbelievable." << std::endl;
        return 1;
    }

    fRtlCompressBuffer = (fpRtlCompressBuffer)GetProcAddress(hNTDLL, "RtlCompressBuffer");
    fRtlDecompressBuffer = (fpRtlDecompressBuffer)GetProcAddress(hNTDLL, "RtlDecompressBuffer");
    fRtlGetCompressionWorkSpaceSize = (fpRtlGetCompressionWorkSpaceSize)GetProcAddress(hNTDLL, "RtlGetCompressionWorkSpaceSize");


    Client client(12345, "192.168.0.4");

    if (!client.is_inited()) {
        return 1;
    }

    std::cout << "Connected to server.\n";


    HDC hdcDesk;
    RECT rect;
    HWND hwndDesk;
    LPCWSTR desktopName = L"test";
    HDESK desktop = NULL;
    HDC hdcScreen;
    HBITMAP bitmap;
    std::vector<BYTE> data, data_compressed;
    ULONG ws_size, fs_size, c_size, uc_size, dc_size;
    NTSTATUS status;
    PVOID  workspace;
    bool code = false;
    {
		if ((desktop = CreateDesktopW(desktopName,
			NULL,
			NULL,
			0,
			GENERIC_ALL,
			NULL)) == NULL)
		{
			goto close;
		}


		hwndDesk = GetDesktopWindow();
		if (GetWindowRect(hwndDesk, &rect) == FALSE)
		{
			goto close;
		}

		hdcDesk = GetDC(hwndDesk);
		hdcScreen = CreateCompatibleDC(hdcDesk);
		bitmap = CreateCompatibleBitmap(hdcDesk, rect.right, rect.bottom);
        SelectObject(hdcScreen, bitmap);
    }


    fRtlGetCompressionWorkSpaceSize(COMPRESSION_FORMAT_LZNT1, &ws_size, &fs_size);
    workspace = (PVOID)new BYTE[ws_size];
;

    while (true) {
        if (!code) {
            client.close();
            if (!client.try_connect()) {
                std::cout << "cant connect\n";
                Sleep(1000);
                continue;
            }
        }

        size_t dwDIBSize = 0;

        {
            BITMAPINFO bi = { 0 };
            BitBlt(hdcScreen, 0, 0, rect.right, rect.bottom, hdcDesk, 0, 0, SRCCOPY);

            bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
            GetDIBits(hdcDesk, bitmap, 0, 0, NULL, &bi, DIB_RGB_COLORS);
            bi.bmiHeader.biCompression = BI_RGB;

            dwDIBSize = ((bi.bmiHeader.biWidth * bi.bmiHeader.biBitCount + 31) & ~31) / 8 * bi.bmiHeader.biHeight;
            data.resize(dwDIBSize);

            GetDIBits(hdcDesk, bitmap, 0, bi.bmiHeader.biHeight, data.data(), (BITMAPINFO*)&bi, DIB_RGB_COLORS);
        }

        /*compress data*/
        {
            data_compressed.resize(dwDIBSize);
            status = fRtlCompressBuffer(
                COMPRESSION_FORMAT_LZNT1,
                data.data(),
                dwDIBSize,
                data_compressed.data(),
                dwDIBSize,
                4096,   // default
                &c_size,
                workspace
            );

            std::cout << c_size << "\n";


            if (status >= 0x8000000) {
                continue;
            }

            code = client.send((const char*)&c_size, sizeof(c_size));
            code = client.send((const char*)data_compressed.data(), c_size);
        }


        Sleep(30);
    }

    delete[] workspace;

close:	
    if (desktop) CloseDesktop(desktop);

    return 0;
}