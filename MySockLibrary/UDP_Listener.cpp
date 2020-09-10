#include "UDP_Listener.h"
#include <strsafe.h>

#define BUF_SIZE 255

bool Keep_Listening(UDP_Listener_Shared_Data* shared_data)
{
    shared_data->mutex.lock();
    bool result = shared_data->run;
    shared_data->mutex.unlock();
    return result;
}

DWORD WINAPI UDP_Listener_Worker(LPVOID lpParam)
{
    // Print to console to verify thread has run (Remove this for release)
    HANDLE hStdout;

    TCHAR msgBuf[BUF_SIZE];
    size_t cchStringSize;
    DWORD dwChars;

    hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hStdout == INVALID_HANDLE_VALUE)
        return 1;

    StringCchPrintf(msgBuf, BUF_SIZE, TEXT("Hello UDP Thread!\n"));
    StringCchLength(msgBuf, BUF_SIZE, &cchStringSize);
    WriteConsole(hStdout, msgBuf, (DWORD)cchStringSize, &dwChars, NULL);

    /////

    UDP_Listener_Shared_Data* shared_data = (UDP_Listener_Shared_Data*)lpParam;

    // Create socket
    SOCKET udp_socket = INVALID_SOCKET;
    udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udp_socket == INVALID_SOCKET) {
        StringCchPrintf(msgBuf, BUF_SIZE, TEXT("create socket failed with error %d\n"), WSAGetLastError());
        StringCchLength(msgBuf, BUF_SIZE, &cchStringSize);
        WriteConsole(hStdout, msgBuf, (DWORD)cchStringSize, &dwChars, NULL);
        return 1;
    }

    // Bind socket
    sockaddr_in server_addr;
    short port = 27015;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(udp_socket, (SOCKADDR*)&server_addr, sizeof(server_addr))) {
        StringCchPrintf(msgBuf, BUF_SIZE, TEXT("bind socket failed with error %d\n"), WSAGetLastError());
        StringCchLength(msgBuf, BUF_SIZE, &cchStringSize);
        WriteConsole(hStdout, msgBuf, (DWORD)cchStringSize, &dwChars, NULL);
        return 1;
    }

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
        FD_SET(udp_socket, &read_fd);
        int status = select(udp_socket + 1, &read_fd, NULL, NULL, &timeout);

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
            bytes_received = recvfrom(udp_socket, buf, buf_len, 0, (SOCKADDR*)&senderAddr, &senderAddrSize);
            if (bytes_received > 0)
            {
                // Add packet to shared_data queue
                //code here...

                // Temp
                buf[bytes_received] = '\0';

                StringCchPrintf(msgBuf, BUF_SIZE, TEXT("got msg %s\n"), buf);
                StringCchLength(msgBuf, BUF_SIZE, &cchStringSize);
                WriteConsole(hStdout, msgBuf, (DWORD)cchStringSize, &dwChars, NULL);
            }
            else if (bytes_received == SOCKET_ERROR) {
                // Log error

                // Close listener
                shared_data->mutex.lock();
                shared_data->run = false;
                shared_data->mutex.unlock();
            }
        }
        else if (status == SOCKET_ERROR)
        {
            // Error looking for new packet

            // Log error
            //code here...

            // Close listener
            shared_data->mutex.lock();
            shared_data->run = false;
            shared_data->mutex.unlock();
        }

        Sleep(100);
    }

    // Close socket

    return 0;
}

UDP_Listener::UDP_Listener()
    : listener_thread(nullptr)
    , listener_thread_id(0)
    , shared_data(new UDP_Listener_Shared_Data())
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

void UDP_Listener::Open()
{
    if (listener_thread)
        throw UDP_Listener_Exception("Could not open udp listener. Listener thread already running.");

    // Set starting values for shared data
    shared_data->run = true;

    listener_thread = CreateThread(
        NULL,                   // default security attributes
        0,                      // use default stack size  
        UDP_Listener_Worker,    // thread function name
        shared_data,            // argument to thread function 
        0,                      // use default creation flags 
        &listener_thread_id);   // returns the thread identifier 

    if (!listener_thread)
        throw UDP_Listener_Exception("Could not open udp listener. Could not start listener thread.");
}

void UDP_Listener::Close()
{
    if (!listener_thread)
        throw UDP_Listener_Exception("Could not close udp listener. Listener thread already closed.");

    // Shutdown tcp listener worker
    shared_data->mutex.lock();
    shared_data->run = false;
    shared_data->mutex.unlock();

    // Wait 3 seconds for tcp listener worker to shutdown
    DWORD wait = WaitForSingleObject(listener_thread, 3000);

    if (wait == WAIT_TIMEOUT)
        throw UDP_Listener_Exception("Could not close udp listener. Timed out.");
    else if (wait != WAIT_OBJECT_0)
        throw UDP_Listener_Exception("Could not close udp listener. Unexpected error.");

    // Close tcp listener worker thread
    CloseHandle(listener_thread);
    listener_thread = nullptr;
}

bool UDP_Listener::Is_Open()
{
    return listener_thread != nullptr;
}
