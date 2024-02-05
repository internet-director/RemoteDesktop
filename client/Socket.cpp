#include "Socket.h"

Socket::Socket(const SOCKET& sock, const sockaddr_in& addr)
{
    init(sock, addr);
}

Socket::~Socket()
{
    close();
}

void Socket::init(const SOCKET& sock, const sockaddr_in& addr)
{
    this->sock = sock;
    this->addr = addr;
    //memcpy(&this->addr, &addr, sizeof(sockaddr_in));
}

void Socket::close()
{
    if (sock != INVALID_SOCKET)
    {
        closesocket(sock);
        sock = INVALID_SOCKET;
    }
}

bool Socket::send(const void* src, size_t len)
{
    if (sock == INVALID_SOCKET)
    {
        return false;
    }

    return ::send(sock, (char*)src, len, 0) != SOCKET_ERROR;
}

size_t Socket::recv(void* dst, size_t len)
{
    if (sock == INVALID_SOCKET)
    {
        return -1;
    }

    return ::recv(sock, (char*)dst, len, 0);
}
