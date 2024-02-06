#pragma once
#include "stdafx.h"

class WSACleaner
{
	WSADATA wsaData;
	bool _inited { true };
	int code{ 0 };

public:
	WSACleaner();
	~WSACleaner();
	bool inited() const noexcept { return _inited; }
	int getErrorCode() const noexcept { return code; }
};

