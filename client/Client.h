#pragma once
#include <WinSock2.h>

class Client
{
	bool inited{ false };
	WSADATA wsaData{ 0 }; 
	SOCKET clientSocket{ INVALID_SOCKET };
	sockaddr_in serverAddr{ 0 };

public:
	Client() = default;
	Client(int port, const char* ip);
	~Client();

	void init(int port, const char* ip);
	bool try_connect();
	bool is_inited() const noexcept { return inited; }
	void close();

	bool send(const char* data, size_t len);
	size_t recv();
};