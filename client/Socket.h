#pragma once
#include "stdafx.h"

struct Socket
{
	Socket() = default;
	Socket(const SOCKET& sock, const sockaddr_in& addr);
	~Socket();

	void init(const SOCKET& sock, const sockaddr_in& addr);
	bool inited() const noexcept { return sock != INVALID_SOCKET; }
	void close();

	bool send(const void* src, size_t len);
	size_t recv(void* dst, size_t len);

protected:
	SOCKET sock{ INVALID_SOCKET };
	sockaddr_in addr{ 0 };
};

