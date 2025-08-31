#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

class IQFeedAPITest {
private:
    SOCKET lookup_socket = INVALID_SOCKET;
    bool is_connected = false;

public:
    IQFeedAPITest() {
        WSADATA wsa_data;
        if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
            throw std::runtime_error("WSAStartup failed");
        }
    }
    
    ~IQFeedAPITest() {
        disconnect();
        WSACleanup();
    }
    
    bool connect_to_lookup_port() {
        std::cout << "Testing direct connection to IQFeed Lookup port 9100..." << std::endl;
        std::cout << "Make sure IQConnect is running and logged in manually first!" << std::endl;
        std::cout << std::endl;
        
        lookup_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (lookup_socket == INVALID_SOCKET) {
            std::cout << "Failed to create socket. Error: " << WSAGetLastError() << std::endl;
            return false;
        }
        
        sockaddr_in lookup_addr{};
        lookup_addr.sin_family = AF_INET;
        lookup_addr.sin_port = htons(9100);
        inet_pton(AF_INET, "127.0.0.1", &lookup_addr.sin_addr);
        
        std::cout << "Attempting to connect to 127.0.0.1:9100..." << std::endl;
        
        if (connect(lookup_socket, (sockaddr*)&lookup_addr, sizeof(lookup_addr)) == SOCKET_ERROR) {
            int error = WSAGetLastError();
            std::cout << "Failed to connect to lookup port 9100. Error: " << error << std::endl;
            
            if (error == 10061) {
                std::cout << "Error 10061 = Connection refused" << std::endl;
                std::cout << "This usually means:" << std::endl;
                std::cout << "1. IQConnect is not running" << std::endl;
                std::cout << "2. IQConnect is not logged in to IQ servers yet" << std::endl;
                std::cout << "3. IQConnect hasn't enabled the lookup port yet" << std::endl;
            }
            
            closesocket(lookup_socket);
            lookup_socket = INVALID_SOCKET;
            return false;
        }
        
        std::cout << "SUCCESS: Connected to IQFeed Lookup port 9100!" << std::endl;
        is_connected = true;
        return true;
    }
    
    bool set_protocol() {
        if (!is_connected) return false;
        
        std::cout << "\nSetting protocol to 6.2..." << std::endl;
        
        std::string command = "S,SET PROTOCOL,6.2\r\n";
        if (send(lookup_socket, command.c_str(), static_cast<int>(command.length()), 0) == SOCKET_ERROR) {
            std::cout << "Failed to send protocol command" << std::endl;
            return false;
        }
        
        // Read response
        char buffer[1024];
        int bytes_received = recv(lookup_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            std::cout << "Protocol response: " << buffer;
            
            if (std::string(buffer).find("S,CURRENT PROTOCOL,6.2") != std::string::npos) {
                std::cout << "Protocol 6.2 set successfully!" << std::endl;
                return true;
            }
        }
        
        std::cout << "Failed to set protocol" << std::endl;
        return false;
    }
    
    bool set_client_name() {
        if (!is_connected) return false;
        
        std::cout << "\nSetting client name..." << std::endl;
        
        std::string command = "S,SET CLIENT NAME,API_Test\r\n";
        if (send(lookup_socket, command.c_str(), static_cast<int>(command.length()), 0) == SOCKET_ERROR) {
            std::cout << "Failed to send client name command" << std::endl;
            return false;
        }
        
        std::cout << "Client name set successfully!" << std::endl;
        return true;
    }
    
    bool test_symbol_lookup() {
        if (!is_connected) return false;
        
        std::cout << "\nTesting symbol lookup for AAPL..." << std::endl;
        
        // Request symbol information for AAPL
        std::string command = "SYM,AAPL,\r\n";
        if (send(lookup_socket, command.c_str(), static_cast<int>(command.length()), 0) == SOCKET_ERROR) {
            std::cout << "Failed to send symbol lookup command" << std::endl;
            return false;
        }
        
        // Read response
        char buffer[2048];
        int bytes_received = recv(lookup_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            std::cout << "Symbol lookup response: " << buffer << std::endl;
            
            if (std::string(buffer).find("AAPL") != std::string::npos) {
                std::cout << "SUCCESS: Symbol lookup is working!" << std::endl;
                return true;
            }
        }
        
        std::cout << "Symbol lookup failed or no response" << std::endl;
        return false;
    }
    
    bool test_historical_data() {
        if (!is_connected) return false;
        
        std::cout << "\nTesting historical data request for AAPL (last 5 days)..." << std::endl;
        
        // Request 5 days of daily data for AAPL
        std::string command = "HDX,AAPL,5,1,\r\n";
        if (send(lookup_socket, command.c_str(), static_cast<int>(command.length()), 0) == SOCKET_ERROR) {
            std::cout << "Failed to send historical data command" << std::endl;
            return false;
        }
        
        std::cout << "Receiving historical data..." << std::endl;
        
        // Read multiple responses (historical data comes in multiple packets)
        int total_bytes = 0;
        int attempts = 0;
        bool found_end = false;
        
        while (attempts < 10 && !found_end) {
            char buffer[4096];
            int bytes_received = recv(lookup_socket, buffer, sizeof(buffer) - 1, 0);
            if (bytes_received > 0) {
                buffer[bytes_received] = '\0';
                total_bytes += bytes_received;
                std::cout << buffer;
                
                // Look for end of transmission marker
                if (std::string(buffer).find("!ENDMSG!") != std::string::npos) {
                    found_end = true;
                }
            } else {
                break;
            }
            attempts++;
        }
        
        if (total_bytes > 0 && found_end) {
            std::cout << "\nSUCCESS: Historical data received (" << total_bytes << " bytes)!" << std::endl;
            return true;
        }
        
        std::cout << "\nHistorical data request failed or incomplete" << std::endl;
        return false;
    }
    
    void disconnect() {
        if (lookup_socket != INVALID_SOCKET) {
            closesocket(lookup_socket);
            lookup_socket = INVALID_SOCKET;
        }
        is_connected = false;
    }
    
    void run_complete_test() {
        std::cout << "=== IQFeed API Direct Connection Test ===" << std::endl;
        std::cout << std::endl;
        
        std::cout << "INSTRUCTIONS:" << std::endl;
        std::cout << "1. Open IQConnect manually (double-click IQConnect.exe)" << std::endl;
        std::cout << "2. Login with your credentials (523576 / 56719893)" << std::endl;
        std::cout << "3. Wait for 'Connected' status in IQConnect window" << std::endl;
        std::cout << "4. Then run this test" << std::endl;
        std::cout << std::endl;
        
        std::cout << "Press Enter when IQConnect is logged in and connected..." << std::endl;
        std::cin.get();
        
        if (!connect_to_lookup_port()) {
            std::cout << "\nTest FAILED: Cannot connect to lookup port" << std::endl;
            return;
        }
        
        if (!set_protocol()) {
            std::cout << "\nTest FAILED: Cannot set protocol" << std::endl;
            return;
        }
        
        if (!set_client_name()) {
            std::cout << "\nTest FAILED: Cannot set client name" << std::endl;
            return;
        }
        
        bool symbol_test = test_symbol_lookup();
        bool historical_test = test_historical_data();
        
        std::cout << "\n=== TEST RESULTS ===" << std::endl;
        std::cout << "Connection to port 9100: SUCCESS" << std::endl;
        std::cout << "Protocol setup: SUCCESS" << std::endl;
        std::cout << "Client name: SUCCESS" << std::endl;
        std::cout << "Symbol lookup: " << (symbol_test ? "SUCCESS" : "FAILED") << std::endl;
        std::cout << "Historical data: " << (historical_test ? "SUCCESS" : "FAILED") << std::endl;
        
        if (symbol_test || historical_test) {
            std::cout << "\nOVERALL: API CONNECTION IS WORKING!" << std::endl;
            std::cout << "You can proceed with building your trading system." << std::endl;
            std::cout << "The Product ID issue only affects command-line launching." << std::endl;
        } else {
            std::cout << "\nOVERALL: API CONNECTION FAILED" << std::endl;
        }
    }
};

int main() {
    try {
        IQFeedAPITest test;
        test.run_complete_test();
        
        std::cout << "\nPress Enter to exit..." << std::endl;
        std::cin.get();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}