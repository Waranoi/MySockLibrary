#pragma once
#undef UNICODE
#define UNICODE
#undef _WINSOCKAPI_
#define _WINSOCKAPI_

#include <windows.h>
#include <stdexcept>
#include <mutex>
#include <vector>
#ifdef _WIN32
#  include <WinSock2.h>
#  pragma comment(lib, "Ws2_32.lib")
#endif

typedef std::function<void(std::vector<char> packet)> On_Packet_Received;

struct UDP_Listener_Shared_Data
{
	std::mutex mutex;
	bool run;
	SOCKET udp_socket;
	std::string worker_error;
	On_Packet_Received packet_recv_callback;

	UDP_Listener_Shared_Data()
	{
		run = false;
		udp_socket = NULL;
		worker_error = "";
		packet_recv_callback = nullptr;
	}
};

class UDP_Listener_Exception : public std::runtime_error {
public:
	UDP_Listener_Exception(const char* message) : std::runtime_error(message) { }
};

class UDP_Listener
{
public:
	UDP_Listener();
	~UDP_Listener();

	void Open(On_Packet_Received new_packet_recv_callback = nullptr, short port = 27015);
	void Close();
	void Close_Forced();
	bool Is_Open();

private:
	void Close_Socket();
	void Close_Thread();

private:
	HANDLE listener_thread;
	DWORD listener_thread_id;

	UDP_Listener_Shared_Data* shared_data;
	bool winsock_init;
};