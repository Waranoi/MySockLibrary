#include <iostream>
#include "Sock_Init.h"
#include "TCP_Listener.h"
#include "UDP_Listener.h"


int main()
{
	try
	{
		// Init winsock
		Sock_Init::Sock_Init();

		// Start tcp listener
		TCP_Listener t;
		t.Open();

		// Start udp listener
		UDP_Listener u;
		u.Open();

		// Pause main thread
		system("pause");
		std::cout << "Hello World!\n";

		// Close listeners
		t.Close();
		u.Close();
	}
	catch (std::runtime_error &exception)
	{
		std::cout << exception.what() << std::endl;
	}

	return 0;
}