#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

int main() {
    // Initialize Winsock
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        std::cout << "WSAStartup failed" << std::endl;
        return 1;
    }
    
    std::cout << "Testing direct connection to IQFeed lookup port 9100..." << std::endl;
    std::cout << "Make sure IQConnect is running and logged in first!" << std::endl;
    std::cout << "Press Enter to continue..." << std::endl;
    std::cin.get();
    
    // Create socket
    SOCKET lookup_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (lookup_socket == INVALID_SOCKET) {
        std::cout << "Failed to create socket: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }
    
    // Connect to lookup port
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9100);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    
    std::cout << "Connecting to 127.0.0.1:9100..." << std::endl;
    
    if (connect(lookup_socket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        int error = WSAGetLastError();
        std::cout << "Connection failed with error: " << error << std::endl;
        if (error == 10061) {
            std::cout << "Connection refused - IQConnect not running or not logged in" << std::endl;
        }
        closesocket(lookup_socket);
        WSACleanup();
        return 1;
    }
    
    std::cout << "Connected successfully!" << std::endl;
    
    // Send protocol command
    std::string protocol_cmd = "S,SET PROTOCOL,6.2\r\n";
    if (send(lookup_socket, protocol_cmd.c_str(), static_cast<int>(protocol_cmd.length()), 0) == SOCKET_ERROR) {
        std::cout << "Failed to send protocol command: " << WSAGetLastError() << std::endl;
        closesocket(lookup_socket);
        WSACleanup();
        return 1;
    }
    
    std::cout << "Protocol command sent, waiting for response..." << std::endl;
    
    // Read protocol response
    char buffer[1024];
    int bytes = recv(lookup_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        std::cout << "Protocol response: " << buffer << std::endl;
    }
    
    // Send historical data request for AAPL
    std::string hist_cmd = "HDX,AAPL,5,0,TEST123,100,1\r\n";
    std::cout << "Sending historical data request for AAPL..." << std::endl;
    
    if (send(lookup_socket, hist_cmd.c_str(), static_cast<int>(hist_cmd.length()), 0) == SOCKET_ERROR) {
        std::cout << "Failed to send historical data request: " << WSAGetLastError() << std::endl;
        closesocket(lookup_socket);
        WSACleanup();
        return 1;
    }
    
    std::cout << "Historical data request sent, waiting for response..." << std::endl;
    
    // Read historical data response
    std::string full_response;
    int attempts = 0;
    while (attempts < 20) {  // Wait up to 10 seconds
        bytes = recv(lookup_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            full_response += buffer;
            std::cout << "Received: " << buffer << std::endl;
            
            if (full_response.find("!ENDMSG!") != std::string::npos) {
                break;
            }
        } else if (bytes == 0) {
            std::cout << "Connection closed by server" << std::endl;
            break;
        } else {
            int error = WSAGetLastError();
            if (error == WSAEWOULDBLOCK) {
                // No data available, wait and try again
                Sleep(500);
                attempts++;
                continue;
            } else {
                std::cout << "Receive error: " << error << std::endl;
                break;
            }
        }
        attempts++;
    }
    
    if (full_response.find("!ENDMSG!") != std::string::npos) {
        std::cout << "\nSUCCESS! Historical data received successfully!" << std::endl;
    } else {
        std::cout << "\nNo complete response received" << std::endl;
    }
    
    closesocket(lookup_socket);
    WSACleanup();
    
    std::cout << "\nPress Enter to exit..." << std::endl;
    std::cin.get();
    
    return 0;
}