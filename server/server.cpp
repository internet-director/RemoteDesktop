#include <iostream>
#include <vector>
#include <thread>
#include <fstream>
#include <format>
#include <WinSock2.h>
#include <Windows.h>

#define CLASS_NAME L"CLASS_NAME"
#pragma comment(lib, "ws2_32.lib")

typedef NTSTATUS(__stdcall* fpRtlDecompressBuffer)(USHORT, PUCHAR, ULONG, PUCHAR, ULONG, PULONG);
typedef NTSTATUS(__stdcall* fpRtlCompressBuffer)(USHORT, PUCHAR, ULONG, PUCHAR, ULONG, ULONG, PULONG, PVOID);
typedef NTSTATUS(__stdcall* fpRtlGetCompressionWorkSpaceSize)(USHORT, PULONG, PULONG);


fpRtlCompressBuffer fRtlCompressBuffer;
fpRtlDecompressBuffer fRtlDecompressBuffer;
fpRtlGetCompressionWorkSpaceSize fRtlGetCompressionWorkSpaceSize;



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
    cls.style = CS_DBLCLKS;
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

    HWND win = CreateWindowExW(WS_EX_TOPMOST,
        CLASS_NAME,
        title,
        WS_OVERLAPPEDWINDOW | WS_THICKFRAME,
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

void LoadScreen(HWND hWnd, HBITMAP bitmap, HDC hdcScreen) {
    /*RECT rect;
    HDC hdc = GetDC(hWnd);
    HDC hdcMem = CreateCompatibleDC(hdc);
    BITMAP bm;

    GetWindowRect(hWnd, &rect);
    GetObjectW(bitmap, sizeof(BITMAP), &bm);

    int w = rect.right - rect.left;
    int h = rect.bottom - rect.top;
    HBITMAP hTempBitmap = CreateCompatibleBitmap(hdc, w, h);
    SelectObject(hdcMem, hTempBitmap);

    StretchBlt(hdcMem, 0, 0, w, h,
        hdcScreen, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);

    HBRUSH brush = CreatePatternBrush(hTempBitmap);
    rect.left = rect.top = 0;
    rect.right = w;
    rect.bottom = h;
    FillRect(hdc, &rect, brush);

    DeleteObject(brush);
    DeleteObject(hTempBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(hWnd, hdc);*/


}

void SaveBitmapToFile(HDC hdc, HBITMAP hBitmap, const char* filename, 
    size_t dwDIBSize, BYTE* lpBits, BITMAPINFO bi)
{
    BITMAP bmp;
    BITMAPFILEHEADER bmfHeader;
    HANDLE hFile;
    DWORD dwBytesWritten;

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


    WriteFile(hFile, lpBits, dwDIBSize, &dwBytesWritten, NULL);

    CloseHandle(hFile);
}


int main()
{
    HDC hdcDesk;
    RECT rect;
    HWND hwndDesk;
    HWND win = NULL;
    HDC hdcScreen;
    HBITMAP bitmap;
    RECT tmp;
    std::vector<char> data, data_decompressed;
    WSADATA wsaData;
    SOCKET listenSocket, clientSocket;
    sockaddr_in serverAddr, clientAddr;
    BYTE buffer[1024];
    std::thread th;
    ULONG ws_size, fs_size, c_size, uc_size, dc_size;
    NTSTATUS status;
    PVOID  workspace;
    std::atomic_bool worker = true;

    fRtlCompressBuffer = (fpRtlCompressBuffer)GetProcAddress(GetModuleHandleA("ntdll.dll"), "RtlCompressBuffer");
    fRtlDecompressBuffer = (fpRtlDecompressBuffer)GetProcAddress(GetModuleHandleA("ntdll.dll"), "RtlDecompressBuffer");
    fRtlGetCompressionWorkSpaceSize = (fpRtlGetCompressionWorkSpaceSize)GetProcAddress(GetModuleHandleA("ntdll.dll"), "RtlGetCompressionWorkSpaceSize");


    int serverPort = 12345;
    int clientAddrLen = sizeof(clientAddr);

    {
        // Initialize Winsock
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "WSAStartup failed.\n";
            return 1;
        }

        // Create socket
        listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listenSocket == INVALID_SOCKET) {
            std::cerr << "Failed to create socket.\n";
            WSACleanup();
            return 1;
        }

        // Server address setup
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(serverPort);

        // Bind socket
        if (bind(listenSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
            std::cerr << "Bind failed.\n";
            closesocket(listenSocket);
            WSACleanup();
            return 1;
        }

        // Listen for incoming connections
        if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
            std::cerr << "Listen failed.\n";
            closesocket(listenSocket);
            WSACleanup();
            return 1;
        }

        clientSocket = accept(listenSocket, reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrLen);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Accept failed.\n";
            closesocket(listenSocket);
            WSACleanup();
            return 1;
        }
    }

    std::cout << "Server listening on port " << serverPort << "...\n";

    hwndDesk = GetDesktopWindow();
    if (GetWindowRect(hwndDesk, &rect) == FALSE)
    {
        goto clean;
    }

    tmp = rect;
    //tmp.right /= 1.5;
    //tmp.bottom /= 1.5;

    W_Register(WindowProc);
    win = W_Create(&tmp);

    if (win == NULL) {
        goto clean;
    }


    //fRtlGetCompressionWorkSpaceSize(COMPRESSION_FORMAT_LZNT1, &ws_size, &fs_size);

    th = std::thread(
        [&]
        {
        while (worker) {
            size_t sz;
            size_t cnt = 0;
            int bytesReceived;

            bytesReceived = recv(clientSocket, (char*)&sz, sizeof(sz), 0);
            
            if (bytesReceived == -1) break;
            data.resize(sz);

            do {
                bytesReceived = recv(clientSocket, (char*)data.data() + cnt, sz - cnt, 0);
                
                if (bytesReceived == -1) break;
                
                cnt += bytesReceived;
                std::cout << cnt << "\n";
            } while (cnt != sz);

            /*decompress data*/
            {
               size_t sz = ((rect.right * 32 + 31) & ~31) / 8 * rect.bottom;

                data_decompressed.resize(sz * 2);
                status = fRtlDecompressBuffer(
                    COMPRESSION_FORMAT_LZNT1,
                    (PUCHAR)data_decompressed.data(),
                    data_decompressed.size(),
                    (PUCHAR)data.data(),
                    cnt,
                    &c_size
                );

                if (status >= 0x8000000) {
                    continue;
                }
            }


            if (cnt != 0) {
                hdcDesk = GetDC(hwndDesk);
                hdcScreen = CreateCompatibleDC(hdcDesk);
                bitmap = CreateCompatibleBitmap(hdcDesk, rect.right, rect.bottom);
                SelectObject(hdcScreen, bitmap);

                

                BITMAPINFO bmpInfo;
                bmpInfo.bmiHeader.biSize = sizeof(bmpInfo.bmiHeader);
                bmpInfo.bmiHeader.biPlanes = 1;
                bmpInfo.bmiHeader.biBitCount = 32;
                bmpInfo.bmiHeader.biCompression = BI_RGB;
                bmpInfo.bmiHeader.biClrUsed = 0;

                bmpInfo.bmiHeader.biWidth = rect.right;
                bmpInfo.bmiHeader.biHeight = rect.bottom;
                bmpInfo.bmiHeader.biSizeImage = 
                    ((bmpInfo.bmiHeader.biWidth * bmpInfo.bmiHeader.biBitCount + 31) & ~31) / 8 * bmpInfo.bmiHeader.biHeight;;

                HDC hWin = GetDC(win);


                /*SaveBitmapToFile(hdcDesk, bitmap,
                    std::format("F:\\tmp\\{}_{}.bmp", time(0), clock()).c_str(),
                    cnt, (BYTE*)data_decompressed.data(), bmpInfo);*/

                SetDIBits(hWin, bitmap, 0, rect.bottom,
                    data_decompressed.data(), &bmpInfo, DIB_RGB_COLORS);

                {
                    HBRUSH brush = CreatePatternBrush(bitmap);
                    FillRect(hWin, &rect, brush);
                    DeleteObject(brush);
                }

                ReleaseDC(win, hWin);
                DeleteDC(hdcScreen);
                DeleteObject(bitmap);
                ReleaseDC(NULL, hdcDesk);
            }
        }
        });
    MSG msg;

    while (GetMessageW(&msg, win, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    worker.store(false);
    th.join();
    // Cleanup client socket
    closesocket(clientSocket);

clean:
    if (win) CloseWindow(win);
    // Cleanup
    closesocket(listenSocket);
    WSACleanup();
    return 0;
}