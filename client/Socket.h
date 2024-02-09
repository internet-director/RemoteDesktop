#pragma once
#include <utility>
#include "stdafx.h"

struct Socket
{
	Socket() = default;
	Socket(const Socket&) = delete;
	Socket(Socket&& other) noexcept;
	Socket(const SOCKET& sock, const sockaddr_in& addr);
	~Socket();

	Socket& operator=(const Socket&) = delete;
	Socket& operator=(Socket&& other) noexcept;

	void init(const SOCKET& sock, const sockaddr_in& addr);
	bool inited() const noexcept { return sock != INVALID_SOCKET; }
	void close();

	bool send(const void* src, size_t len);
	bool recv(void* dst, size_t len);

	void swap(Socket&& other) {
		std::swap(this->addr, other.addr);
		std::swap(this->sock, other.sock);
	}

protected:
	SOCKET sock{ INVALID_SOCKET };
	sockaddr_in addr{ 0 };
};

