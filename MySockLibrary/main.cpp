#include <iostream>
#include "TCP_Listener.h"
#include "UDP_Listener.h"

std::vector<std::vector<char>> packet_list;

void packet_recv(std::vector<char> packet)
{
	// This runs on the same thread as the udp listener, so be careful with that
	packet_list.push_back(packet);
}

int main()
{
	try
	{
		// Start udp listener
		UDP_Listener u;
		u.Open(&packet_recv);

		// Pause main thread
		system("pause");
		std::cout << "Hello World!\n";

		// Close listeners
		u.Close();

		// Print received packets
		for (std::vector<char> packet : packet_list)
		{
			for (char c : packet)
			{
				std::cout << c;
			}
			std::cout << "\n";
		}
	}
	catch (std::runtime_error &exception)
	{
		std::cout << exception.what() << std::endl;
	}

	return 0;
}