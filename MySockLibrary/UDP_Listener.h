#pragma once
#include <windows.h>
#include <stdexcept>
#include <mutex>

struct UDP_Listener_Shared_Data
{
	std::mutex mutex;
	bool run;
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

	void Open();
	void Close();
	bool Is_Open();

	//private:
	HANDLE listener_thread;
	DWORD listener_thread_id;

	UDP_Listener_Shared_Data* shared_data;
};