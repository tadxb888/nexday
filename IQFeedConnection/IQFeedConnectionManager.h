#ifndef IQFEED_CONNECTION_MANAGER_H
#define IQFEED_CONNECTION_MANAGER_H

#include <string>
#include <memory>
#include "Logger.h"  // Include instead of forward declaration

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#endif

class IQFeedConnectionManager {
private:
    static const int LOOKUP_PORT = 9100;
    std::unique_ptr<Logger> logger;
    bool winsock_initialized = false;
    bool is_connected = false;

public:
    IQFeedConnectionManager();
    ~IQFeedConnectionManager();
    
    // Connection management
    bool initialize_connection();
    void shutdown_connection();
    bool is_connection_ready() const;
    
    // Socket creation for data requests
    SOCKET create_lookup_socket();
    void close_lookup_socket(SOCKET socket);
    
    // Send commands and read responses
    bool send_command(SOCKET socket, const std::string& command);
    std::string read_full_response(SOCKET socket);

private:
    void initialize_winsock();
    void cleanup_winsock();
    bool test_connection();
    int get_last_error();
};

#endif // IQFEED_CONNECTION_MANAGER_H