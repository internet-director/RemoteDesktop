#pragma once
#include "../client/stdafx.h"
#include "../client/Socket.h"

struct Server: public Socket
{
	Server() = default;
	Server(int port);

	bool init(int port);
	bool accept(Socket& client);
};
