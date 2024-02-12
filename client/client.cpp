#include "Client.h"

Client::Client(int port, const char* ip)
{
	init(port, ip);
}

bool Client::init(int port, const char* ip)
{
	close();
	addr.sin_family = AF_INET;
	inet_pton(AF_INET, ip, &addr.sin_addr.s_addr);
	addr.sin_port = htons(port);
	return true;
}

bool Client::try_connect()
{
	if (sock == INVALID_SOCKET)
	{
		if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
		{
			return false;
		}
	}

	return connect(sock, reinterpret_cast< sockaddr* >(&addr), sizeof(addr)) != SOCKET_ERROR;
}