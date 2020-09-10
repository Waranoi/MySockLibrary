#pragma once

#include <windows.h>
#include <stdexcept>
#include <mutex>

struct TCP_Listener_Shared_Data
{
	std::mutex mutex;
	bool run;
};

class TCP_Listener_Exception : public std::runtime_error {
public:
	TCP_Listener_Exception(const char* message) : std::runtime_error(message) { }
};

class TCP_Listener
{
public:
	TCP_Listener();
	~TCP_Listener();

	void Open();
	void Close();
	bool Is_Open();

private:
	HANDLE listener_thread;
	DWORD listener_thread_id;
	
	TCP_Listener_Shared_Data *shared_data;
};