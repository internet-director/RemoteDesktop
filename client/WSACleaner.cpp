#include "WSACleaner.h"

WSACleaner::WSACleaner()
{
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		_inited = false;
		code = WSAGetLastError();
	}
}

WSACleaner::~WSACleaner() 
{
	WSACleanup();
}
