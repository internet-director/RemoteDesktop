#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "Client.h"

Client::Client(int port, const char* ip)
{
    init(port, ip);
}

Client::~Client()
{
    close();
}

bool Client::init(int port, const char* ip)
{
    close();
    serverAddr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &serverAddr.sin_addr.s_addr);
    serverAddr.sin_port = htons(port);
    _inited = true;
    return true;
}

bool Client::try_connect()
{
    if (clientSocket == INVALID_SOCKET)
    {
        if ((clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
        {
            return false;
        }
    }

    return connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) != SOCKET_ERROR;
}

void Client::close()
{
    if (clientSocket != INVALID_SOCKET) {
        closesocket(clientSocket);
        clientSocket = INVALID_SOCKET;
    }
}

bool Client::send(const char* data, size_t len)
{
    if (clientSocket == INVALID_SOCKET)
    {
        return false;
    }

    return ::send(clientSocket, (const char*)data, len, 0) != SOCKET_ERROR;
}

size_t Client::recv()
{
    return size_t();
}
