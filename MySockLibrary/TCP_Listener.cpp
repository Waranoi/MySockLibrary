#include "TCP_Listener.h"
#include <strsafe.h>

#define BUF_SIZE 255

bool Keep_Listening(TCP_Listener_Shared_Data* shared_data)
{
    shared_data->mutex.lock();
    bool result = shared_data->run;
    shared_data->mutex.unlock();
    return result;
}

DWORD WINAPI TCP_Listener_Worker(LPVOID lpParam)
{
    // Print to console to verify thread has run (Remove this for release)
    HANDLE hStdout;

    TCHAR msgBuf[BUF_SIZE];
    size_t cchStringSize;
    DWORD dwChars;

    hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hStdout == INVALID_HANDLE_VALUE)
        return 1;

    StringCchPrintf(msgBuf, BUF_SIZE, TEXT("Hello TCP Thread!\n"));
    StringCchLength(msgBuf, BUF_SIZE, &cchStringSize);
    WriteConsole(hStdout, msgBuf, (DWORD)cchStringSize, &dwChars, NULL);

    //////

    TCP_Listener_Shared_Data* shared_data = (TCP_Listener_Shared_Data*)lpParam;

    // Create socket

    // Accept incoming connections
    while (Keep_Listening(shared_data))
    {


        Sleep(100);
    }

    // Close socket

    return 0;
}

TCP_Listener::TCP_Listener()
    : listener_thread(nullptr)
    , listener_thread_id(0)
    , shared_data(new TCP_Listener_Shared_Data())
{

}

TCP_Listener::~TCP_Listener()
{
    try
    {
        Close();
    }
    catch (...)
    { }

    if (shared_data)
    {
        delete shared_data;
        shared_data = nullptr;
    }
}

void TCP_Listener::Open()
{
    if (listener_thread)
        throw TCP_Listener_Exception("Could not open tcp listener. Listener thread already running.");

    // Set starting values for shared data
    shared_data->run = true;

    listener_thread = CreateThread(
        NULL,                   // default security attributes
        0,                      // use default stack size  
        TCP_Listener_Worker,    // thread function name
        shared_data,            // argument to thread function 
        0,                      // use default creation flags 
        &listener_thread_id);   // returns the thread identifier 

    if (!listener_thread)
        throw TCP_Listener_Exception("Could not open tcp listener. Could not start listener thread.");
}

void TCP_Listener::Close()
{
    if (!listener_thread)
        throw TCP_Listener_Exception("Could not close tcp listener. Listener thread already closed.");

    // Shutdown tcp listener worker
    shared_data->mutex.lock();
    shared_data->run = false;
    shared_data->mutex.unlock();

    // Wait 3 seconds for tcp listener worker to shutdown
    DWORD wait = WaitForSingleObject(listener_thread, 3000);

    if (wait == WAIT_TIMEOUT)
        throw TCP_Listener_Exception("Could not close tcp listener. Timed out.");
    else if (wait != WAIT_OBJECT_0)
        throw TCP_Listener_Exception("Could not close tcp listener. Unexpected error.");

    // Close tcp listener worker thread
    CloseHandle(listener_thread);
    listener_thread = nullptr;
}

bool TCP_Listener::Is_Open()
{
    return listener_thread != nullptr;
}
