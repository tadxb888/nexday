#include "IQFeedConnectionManager.h"
#include "Logger.h"
#include <iostream>
#include <thread>
#include <chrono>

IQFeedConnectionManager::IQFeedConnectionManager() {
    logger = std::make_unique<Logger>("iqfeed_connection.log", true);
    initialize_winsock();
}

IQFeedConnectionManager::~IQFeedConnectionManager() {
    shutdown_connection();
    cleanup_winsock();
}

bool IQFeedConnectionManager::initialize_connection() {
    logger->info("Initializing IQFeed connection...");
    
    // Test connection to ensure IQConnect is running
    if (!test_connection()) {
        logger->error("Failed to establish test connection to IQFeed");
        return false;
    }
    
    is_connected = true;
    logger->success("IQFeed connection initialized successfully");
    return true;
}

void IQFeedConnectionManager::shutdown_connection() {
    if (is_connected) {
        logger->info("Shutting down IQFeed connection");
        is_connected = false;
    }
}

bool IQFeedConnectionManager::is_connection_ready() const {
    return is_connected;
}

SOCKET IQFeedConnectionManager::create_lookup_socket() {
    logger->debug("Creating lookup socket...");
    
    SOCKET lookup_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (lookup_socket == INVALID_SOCKET) {
        logger->error("Failed to create socket. Error: " + std::to_string(get_last_error()));
        return INVALID_SOCKET;
    }
    
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(LOOKUP_PORT);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    
    if (connect(lookup_socket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        int error = get_last_error();
        logger->error("Connection failed with error: " + std::to_string(error));
        if (error == 10061) {
            logger->error("Connection refused - IQConnect not running or not logged in");
        }
        closesocket(lookup_socket);
        return INVALID_SOCKET;
    }
    
    // Set protocol
    std::string protocol_cmd = "S,SET PROTOCOL,6.2\r\n";
    if (send(lookup_socket, protocol_cmd.c_str(), static_cast<int>(protocol_cmd.length()), 0) == SOCKET_ERROR) {
        logger->error("Failed to send protocol command");
        closesocket(lookup_socket);
        return INVALID_SOCKET;
    }
    
    // Read protocol response
    char buffer[1024];
    int bytes = recv(lookup_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        logger->debug("Protocol response: " + std::string(buffer));
    }
    
    logger->debug("Lookup socket created and configured successfully");
    return lookup_socket;
}

void IQFeedConnectionManager::close_lookup_socket(SOCKET socket) {
    if (socket != INVALID_SOCKET) {
        closesocket(socket);
        logger->debug("Lookup socket closed");
    }
}

bool IQFeedConnectionManager::send_command(SOCKET socket, const std::string& command) {
    logger->debug("Sending command: " + command);
    
    if (send(socket, command.c_str(), static_cast<int>(command.length()), 0) == SOCKET_ERROR) {
        logger->error("Failed to send command. Error: " + std::to_string(get_last_error()));
        return false;
    }
    
    return true;
}

std::string IQFeedConnectionManager::read_full_response(SOCKET socket) {
    std::string full_response;
    char buffer[4096];
    int attempts = 0;
    const int max_attempts = 60; // 30 seconds timeout
    
    while (attempts < max_attempts) {
        int bytes = recv(socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            full_response += std::string(buffer);
            
            // Check if we received the end message
            if (full_response.find("!ENDMSG!") != std::string::npos) {
                break;
            }
            
            attempts = 0; // Reset timeout if we received data
        } else if (bytes == 0) {
            logger->debug("Connection closed by server");
            break;
        } else {
            int error = get_last_error();
            if (error == WSAEWOULDBLOCK || error == WSAETIMEDOUT) {
                // No data available, wait and try again
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                attempts++;
                continue;
            } else {
                logger->error("Receive error: " + std::to_string(error));
                break;
            }
        }
    }
    
    if (attempts >= max_attempts) {
        logger->error("Timeout waiting for complete response");
    }
    
    return full_response;
}

void IQFeedConnectionManager::initialize_winsock() {
#ifdef _WIN32
    WSADATA wsa_data;
    int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (result != 0) {
        throw std::runtime_error("WSAStartup failed: " + std::to_string(result));
    }
    winsock_initialized = true;
    logger->debug("Winsock initialized successfully");
#endif
}

void IQFeedConnectionManager::cleanup_winsock() {
#ifdef _WIN32
    if (winsock_initialized) {
        WSACleanup();
        logger->debug("Winsock cleaned up");
    }
#endif
}

bool IQFeedConnectionManager::test_connection() {
    SOCKET test_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (test_socket == INVALID_SOCKET) {
        return false;
    }
    
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(LOOKUP_PORT);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    
    bool connection_ok = (connect(test_socket, (sockaddr*)&addr, sizeof(addr)) == 0);
    closesocket(test_socket);
    
    return connection_ok;
}

int IQFeedConnectionManager::get_last_error() {
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}