#pragma once

#undef UNICODE
#define UNICODE
#undef _WINSOCKAPI_
#define _WINSOCKAPI_

#include <stdexcept>
#ifdef _WIN32
#  include <WinSock2.h>
#  pragma comment(lib, "Ws2_32.lib")
#endif

class Sock_Init_Exception : public std::runtime_error {
public:
	Sock_Init_Exception(const char* message) : std::runtime_error(message) { }
};

namespace Sock_Init
{
	inline void Sock_Init()
	{
		WSADATA wsaData;
		int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (res != NO_ERROR)
			throw Sock_Init_Exception("Could not initialize WinSock library.");
	}
};