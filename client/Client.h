#pragma once
#include "Socket.h"
#include "stdafx.h"

struct Client : public Socket
{
	Client() = default;
	Client(int port, const char* ip);

	bool init(int port, const char* ip);
	bool try_connect();
};