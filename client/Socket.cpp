#include "Socket.h"

Socket::Socket(Socket&& other) noexcept {
    if (&other == this) {
        return;
    }
    this->swap(std::move(other));
}

Socket::Socket(const SOCKET& sock, const sockaddr_in& addr)
{
    init(sock, addr);
}

Socket::~Socket()
{
    close();
}

inline Socket& Socket::operator=(Socket&& other) noexcept {
    if (&other == this) {
        return *this;
    }

    Socket s(std::move(other));
    this->swap(std::move(s));
    return *this;
}

void Socket::init(const SOCKET& sock, const sockaddr_in& addr)
{
    this->sock = sock;
    this->addr = addr;
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

    size_t counter{ 0 };

    do
    {
        int sended = ::send(sock, (char*)src + counter, len - counter, 0);

        if (sended == SOCKET_ERROR)
        {
            return false;
        }
        counter += sended;
    } while (counter != len);

    return true;
}

bool Socket::recv(void* dst, size_t len)
{
    if (sock == INVALID_SOCKET)
    {
        return -1;
    }

    size_t counter{ 0 };

    do
    {
        int sended = ::recv(sock, (char*)dst + counter, len - counter, 0);

        if (sended == SOCKET_ERROR)
        {
            return false;
        }

        counter += sended;
    } while (counter != len);

    return true;
}
