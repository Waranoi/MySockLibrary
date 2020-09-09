#include <iostream>
#include "TCP_Listener.h"

int main()
{
	TCP_Listener t;
	t.Open();
	t.Close();

	std::cout << "Hello World!\n";
	return 0;
}