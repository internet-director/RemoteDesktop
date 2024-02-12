#pragma once
#include "../client/Socket.h"
#include "../client/stdafx.h"

struct Server : public Socket
{
	Server() = default;
	Server(int port);

	bool init(int port);
	bool accept(Socket& client);
};
