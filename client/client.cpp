#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
#include <string>
#include <WinSock2.h>
#include <Windows.h>
#include <thread>
#include <format>
#include <fstream>
#include <vector>
#include <gdiplus.h>
#include <GdiPlusFlat.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment (lib,"Gdiplus.lib")

typedef NTSTATUS(__stdcall* fpRtlDecompressBuffer)(USHORT, PUCHAR, ULONG, PUCHAR, ULONG, PULONG);
typedef NTSTATUS(__stdcall* fpRtlCompressBuffer)(USHORT, PUCHAR, ULONG, PUCHAR, ULONG, ULONG, PULONG, PVOID);
typedef NTSTATUS(__stdcall* fpRtlGetCompressionWorkSpaceSize)(USHORT, PULONG, PULONG);


fpRtlCompressBuffer fRtlCompressBuffer;
fpRtlDecompressBuffer fRtlDecompressBuffer;
fpRtlGetCompressionWorkSpaceSize fRtlGetCompressionWorkSpaceSize;

void SaveBitmapToFile(HDC hdc, HBITMAP hBitmap, const char* filename) {
    BITMAP bmp;
    BITMAPINFO bi = { 0 };
    BITMAPFILEHEADER bmfHeader;
    DWORD dwDIBSize;
    HANDLE hFile;
    DWORD dwBytesWritten;

    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    GetDIBits(hdc, hBitmap, 0, 0, NULL, &bi, DIB_RGB_COLORS);

    hFile = CreateFileA(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to create file." << std::endl;
        return;
    }

    dwDIBSize = ((bi.bmiHeader.biWidth * bi.bmiHeader.biBitCount + 31) & ~31) / 8 * bi.bmiHeader.biHeight;

    bmfHeader.bfType = 0x4D42; // "BM"
    bmfHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwDIBSize;
    bmfHeader.bfReserved1 = 0;
    bmfHeader.bfReserved2 = 0;
    bmfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    bi.bmiHeader.biCompression = BI_RGB;

    WriteFile(hFile, &bmfHeader, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
    WriteFile(hFile, &bi.bmiHeader, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);

    BYTE* lpBits = new BYTE[dwDIBSize];

    if (!lpBits) {
        std::cerr << "Failed to allocate memory." << std::endl;
        CloseHandle(hFile);
        return;
    }

    GetDIBits(hdc, hBitmap, 0, bi.bmiHeader.biHeight, lpBits, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

    WriteFile(hFile, lpBits, dwDIBSize, &dwBytesWritten, NULL);

    delete[] lpBits;
    CloseHandle(hFile);
}

int main()
{
    WSADATA wsaData;
    SOCKET clientSocket;
    sockaddr_in serverAddr;
    int serverPort = 12345;


    fRtlCompressBuffer = (fpRtlCompressBuffer)GetProcAddress(GetModuleHandleA("ntdll.dll"), "RtlCompressBuffer");
    fRtlDecompressBuffer = (fpRtlDecompressBuffer)GetProcAddress(GetModuleHandleA("ntdll.dll"), "RtlDecompressBuffer");
    fRtlGetCompressionWorkSpaceSize = (fpRtlGetCompressionWorkSpaceSize)GetProcAddress(GetModuleHandleA("ntdll.dll"), "RtlGetCompressionWorkSpaceSize");


    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed.\n";
        return 1;
    }

    // Create socket
    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Failed to create socket.\n";
        WSACleanup();
        return 1;
    }

    // Server address setup
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("192.168.0.4"); // Change to the server's IP address
    serverAddr.sin_port = htons(serverPort);

    // Connect to server
    if (connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Failed to connect to server.\n";
        closesocket(clientSocket);
        WSACleanup();
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

    {
		if ((desktop = CreateDesktopW(desktopName,
			NULL,
			NULL,
			0,
			GENERIC_ALL,
			NULL)) == NULL)
		{
			goto clean;
		}


		hwndDesk = GetDesktopWindow();
		if (GetWindowRect(hwndDesk, &rect) == FALSE)
		{
			goto clean;
		}

		hdcDesk = GetDC(hwndDesk);
		hdcScreen = CreateCompatibleDC(hdcDesk);
		bitmap = CreateCompatibleBitmap(hdcDesk, rect.right, rect.bottom);
        SelectObject(hdcScreen, bitmap);
    }


    fRtlGetCompressionWorkSpaceSize(COMPRESSION_FORMAT_LZNT1, &ws_size, &fs_size);
    workspace = (PVOID)new BYTE[ws_size];

    while (true) {
        // Read message from console
        std::string message;

        size_t dwDIBSize = 0;

        {
            BITMAPINFO bi = { 0 };
            BitBlt(hdcScreen, 0, 0, rect.right, rect.bottom, hdcDesk, 0, 0, SRCCOPY);
            //SaveBitmapToFile(hdcDesk, bitmap, std::format("F:\\tmp\\{}_{}.bmp", time(0), clock()).c_str());
            DWORD dwBytesWritten;

            bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
            GetDIBits(hdcDesk, bitmap, 0, 0, NULL, &bi, DIB_RGB_COLORS);


            dwDIBSize = ((bi.bmiHeader.biWidth * bi.bmiHeader.biBitCount + 31) & ~31) / 8 * bi.bmiHeader.biHeight;
            bi.bmiHeader.biCompression = BI_RGB;

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
                2048,
                &c_size,
                workspace
            );

            std::cout << c_size << "\n";

            if (status >= 0x8000000) {
                send(clientSocket, (const char*)&dwDIBSize, sizeof(dwDIBSize), 0);
                send(clientSocket, (const char*)data.data(), dwDIBSize, 0);
            }
            else {
                dwDIBSize = c_size;
                send(clientSocket, (const char*)&dwDIBSize, sizeof(dwDIBSize), 0);
                send(clientSocket, (const char*)data_compressed.data(), c_size, 0);
            }
        }
        Sleep(30);
    }

    delete[] workspace;

clean:	
    if (desktop) CloseDesktop(desktop);
    // Cleanup
    closesocket(clientSocket);
    WSACleanup();

    return 0;
}