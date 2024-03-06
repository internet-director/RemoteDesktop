#pragma once
#include "stdafx.h"

enum CLIENT_COMMAND
{
	NONE = 0,
	RUN = 1,
	STOP = 2,
	SHUTDOWN = 3,
	RESTART = 4,
	GET_STATUS = 5,
	GET_CONFIGURATION = 6
};

typedef struct
{
	int width;
	int height;
	int size;
	int c_size;
} FRAME_INFO, *PFRAME_INFO;

typedef struct
{
	CLIENT_COMMAND command;
} COMMAND_EXECUTOR;

typedef struct
{
	int monitorCount;
} CLIENT_CONFIGURATION, *PCLIENT_CONFIGURATION;

typedef struct
{
	int size;
} SERVER_CONFIGURATION, *PSERVER_CONFIGURATION;
