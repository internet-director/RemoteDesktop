#include "Server.h"

Server::Server(int port)
{
	init(port);
}

bool Server::init(int port)
{
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (sock == INVALID_SOCKET)
	{
		return false;
	}

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);

	if (bind(sock, reinterpret_cast< sockaddr* >(&addr), sizeof(addr)) == SOCKET_ERROR)
	{
		close();
		return false;
	}

	if (listen(sock, SOMAXCONN) == SOCKET_ERROR)
	{
		close();
		return false;
	}

	return true;
}

bool Server::accept(Socket& client)
{
	if (sock == INVALID_SOCKET)
	{
		return false;
	}

	client.close();

	sockaddr_in clientAddr{ 0 };
	int clientAddrLen = sizeof(clientAddr);
	SOCKET clientSocket = ::accept(sock, reinterpret_cast< sockaddr* >(&clientAddr), &clientAddrLen);

	if (clientSocket == INVALID_SOCKET)
	{
		return false;
	}

	client.init(clientSocket, clientAddr);
	return true;
}
