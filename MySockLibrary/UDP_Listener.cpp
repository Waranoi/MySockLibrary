#undef UNICODE
#define UNICODE
#undef _WINSOCKAPI_
#define _WINSOCKAPI_

#include "UDP_Listener.h"
#include <strsafe.h>
#ifdef _WIN32
#  include <WinSock2.h>
#  pragma comment(lib, "Ws2_32.lib")
#endif

bool Keep_Listening(UDP_Listener_Shared_Data* shared_data)
{
    shared_data->mutex.lock();
    bool result = shared_data->run;
    shared_data->mutex.unlock();
    return result;
}

DWORD WINAPI UDP_Listener_Worker(LPVOID lpParam)
{
    UDP_Listener_Shared_Data* shared_data = (UDP_Listener_Shared_Data*)lpParam;

    // select() query variables
    fd_set read_fd;
    timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 30;

    // Accept incoming packets
    while (Keep_Listening(shared_data))
    {
        // Query for new packets
        FD_ZERO(&read_fd);
        FD_SET(shared_data->udp_socket, &read_fd);
        int status = select(shared_data->udp_socket + 1, &read_fd, NULL, NULL, &timeout);

        if (status > 0)
        {
            // Found new packet

            // Packet information
            char buf[1025];
            int buf_len = 1024;
            int bytes_received;

            // Sender information
            sockaddr_in senderAddr;
            int senderAddrSize = sizeof(senderAddr);

            // Receive packet
            bytes_received = recvfrom(shared_data->udp_socket, buf, buf_len, 0, (SOCKADDR*)&senderAddr, &senderAddrSize);
            if (bytes_received > 0)
            {
                // Add packet to shared_data queue
                shared_data->mutex.lock();
                if (shared_data->packet_recv_callback)
                    shared_data->packet_recv_callback(std::vector<char>(buf, buf + bytes_received));
                shared_data->mutex.unlock();
            }
            else if (bytes_received == SOCKET_ERROR) {
                // Log error and close listener
                shared_data->mutex.lock();
                shared_data->worker_error = "Could not listen on udp listener. Could not receive packet from udp socket.";
                shared_data->run = false;
                shared_data->mutex.unlock();
            }
        }
        else if (status == SOCKET_ERROR)
        {
            // Error looking for new packet
            // Log error and close listener
            shared_data->mutex.lock();
            shared_data->worker_error = "Could not listen on udp listener. Could not query packets from udp socket.";
            shared_data->run = false;
            shared_data->mutex.unlock();
        }

        Sleep(100);
    }

    return 0;
}

UDP_Listener::UDP_Listener()
    : listener_thread(nullptr)
    , listener_thread_id(0)
    , shared_data(new UDP_Listener_Shared_Data())
    , winsock_init(false)
{
}

UDP_Listener::~UDP_Listener()
{
    try
    {
        Close();
    }
    catch (...)
    {

    }
    if (shared_data)
    {
        delete shared_data;
        shared_data = nullptr;
    }
}

void UDP_Listener::Open(On_Packet_Received new_packet_recv_callback, short port)
{
    if (listener_thread)
        throw UDP_Listener_Exception("Could not open udp listener. Listener thread already running.");

    if (winsock_init)
        throw UDP_Listener_Exception("Could not open udp listener. Winsock already initialized.");

    WSADATA wsaData;
    int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (res != NO_ERROR)
        throw UDP_Listener_Exception("Could not open udp listener. Could not initialize WinSock library.");
    winsock_init = true;

    // Create socket
    shared_data->udp_socket = INVALID_SOCKET;
    shared_data->udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (shared_data->udp_socket == INVALID_SOCKET) {
        char error_buf[256];
        sprintf_s(error_buf, 256, "Could not open udp listener. Failed to create udp socket with error %d.", WSAGetLastError());
        UDP_Listener_Exception exception = UDP_Listener_Exception(error_buf);

        try
        {
            Close();
        }
        catch (UDP_Listener_Exception& close_exception)
        {
            char error_buf[256];
            sprintf_s(error_buf, 256, "WARNING. Could not open udp listener. Could not clean up after failing to open udp listener.\n%s\n%s", close_exception.what(), exception.what());
            exception = UDP_Listener_Exception(error_buf);
        }
        throw exception;
    }

    // Bind socket
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(shared_data->udp_socket, (SOCKADDR*)&server_addr, sizeof(server_addr))) {
        char error_buf[256];
        sprintf_s(error_buf, 256, "Could not open udp listener. Failed to bind udp socket with error %d.", WSAGetLastError());
        UDP_Listener_Exception exception = UDP_Listener_Exception(error_buf);

        try
        {
            Close();
        }
        catch (UDP_Listener_Exception& close_exception)
        {
            char error_buf[256];
            sprintf_s(error_buf, 256, "WARNING. Could not open udp listener. Could not clean up after failing to open udp listener.\n%s\n%s", close_exception.what(), exception.what());
            exception = UDP_Listener_Exception(error_buf);
        }
        throw exception;
    }

    // Keep listener running
    shared_data->run = true;

    // Set on packet received callback function
    shared_data->packet_recv_callback = new_packet_recv_callback;

    listener_thread = CreateThread(
        NULL,                   // default security attributes
        0,                      // use default stack size  
        UDP_Listener_Worker,    // thread function name
        shared_data,            // argument to thread function 
        0,                      // use default creation flags 
        &listener_thread_id);   // returns the thread identifier 

    if (!listener_thread)
    {
        UDP_Listener_Exception exception = UDP_Listener_Exception("Could not open udp listener. Could not start listener thread.");

        try
        {
            Close();
        }
        catch (UDP_Listener_Exception& close_exception)
        {
            char error_buf[256];
            sprintf_s(error_buf, 256, "WARNING. Could not open udp listener. Could not clean up after failing to open udp listener.\n%s\n%s", close_exception.what(), exception.what());
            exception = UDP_Listener_Exception(error_buf);
        }
        throw exception;
    }
}

void UDP_Listener::Close()
{
    Close_Thread();
    Close_Socket();

    shared_data->packet_recv_callback = nullptr;
}

void UDP_Listener::Close_Forced()
{
    if (listener_thread)
    {
        // Shutdown tcp listener worker
        shared_data->mutex.lock();
        shared_data->run = false;
        shared_data->mutex.unlock();

        // Wait 3 seconds for tcp listener worker to shutdown
        WaitForSingleObject(listener_thread, 3000);

        // Close tcp listener worker thread
        CloseHandle(listener_thread);
        listener_thread = nullptr;
    }

    if (shared_data->udp_socket != NULL)
    {
        if (shared_data->udp_socket != INVALID_SOCKET)
            closesocket(shared_data->udp_socket);
        shared_data->udp_socket = NULL;
    }

    shared_data->packet_recv_callback = nullptr;

    if (winsock_init)
    {
        WSACleanup();
        winsock_init = false;
    }
}

bool UDP_Listener::Is_Open()
{
    return listener_thread != nullptr;
}

void UDP_Listener::Close_Socket()
{
    if (shared_data->udp_socket != NULL)
    {
        if (shared_data->udp_socket != INVALID_SOCKET && closesocket(shared_data->udp_socket) != 0)
            throw UDP_Listener_Exception("Could not close udp listener. Could not close socket.");
        shared_data->udp_socket = NULL;
    }

    if (winsock_init)
    {
        int res = WSACleanup();
        if (res != NO_ERROR)
            throw UDP_Listener_Exception("Could not close udp listener. Could not terminate WinSock library.");
        winsock_init = false;
    }
}

void UDP_Listener::Close_Thread()
{
    if (listener_thread)
    {
        // Shutdown udp listener worker
        shared_data->mutex.lock();
        shared_data->run = false;
        shared_data->mutex.unlock();

        // Wait 3 seconds for udp listener worker to shutdown
        DWORD wait = WaitForSingleObject(listener_thread, 3000);

        if (wait == WAIT_TIMEOUT)
            throw UDP_Listener_Exception("Could not close udp listener. Timed out.");
        else if (wait != WAIT_OBJECT_0)
            throw UDP_Listener_Exception("Could not close udp listener. Unexpected error.");

        // Close udp listener worker thread
        if (!CloseHandle(listener_thread))
            throw UDP_Listener_Exception("Could not close udp listener. Could not close listener thread.");
        listener_thread = nullptr;
    }
}
