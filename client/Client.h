#pragma once
#include "stdafx.h"

class Client
{
	bool _inited{ false };
	SOCKET clientSocket{ INVALID_SOCKET };
	sockaddr_in serverAddr{ 0 };

public:
	Client() = default;
	Client(int port, const char* ip);
	~Client();

	bool init(int port, const char* ip);
	bool try_connect();
	bool inited() const noexcept { return _inited; }
	void close();

	bool send(const char* data, size_t len);
	size_t recv();
};