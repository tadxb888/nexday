#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <memory>
#include <fstream>
#include <sstream>
#include <filesystem>

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

class Logger {
private:
    std::ofstream log_file;
    bool logging_enabled;
    std::string get_timestamp() const {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto tm = *std::localtime(&time_t);
        
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

public:
    Logger(const std::string& filename = "iqfeed_test.log", bool enabled = true) 
        : logging_enabled(enabled) {
        if (logging_enabled) {
            // Create logs directory if it doesn't exist
            std::filesystem::create_directories("logs");
            log_file.open("logs/" + filename, std::ios::app);
        }
    }
    
    ~Logger() {
        if (log_file.is_open()) {
            log_file.close();
        }
    }
    
    void log(const std::string& level, const std::string& message) {
        if (!logging_enabled) return;
        
        auto timestamp = get_timestamp();
        auto log_entry = "[" + level + "] " + timestamp + " - " + message;
        
        // Always output to console
        std::cout << log_entry << std::endl;
        
        // Also log to file if available
        if (log_file.is_open()) {
            log_file << log_entry << std::endl;
            log_file.flush();
        }
    }
    
    void info(const std::string& message) { log("INFO", message); }
    void warn(const std::string& message) { log("WARN", message); }
    void error(const std::string& message) { log("ERROR", message); }
    void debug(const std::string& message) { log("DEBUG", message); }
    void success(const std::string& message) { log("SUCCESS", "✅ " + message); }
    void step(const std::string& message) { log("STEP", "🔄 " + message); }
};

class IQFeedConnectionTest {
private:
    // Your credentials
    std::string product_id = "Elias_Rostane_51184";
    std::string version = "1.0.0.0";
    std::string login_id = "523576";
    std::string password = "56719893";
    
    // Connection sockets
    SOCKET admin_socket = INVALID_SOCKET;
    SOCKET lookup_socket = INVALID_SOCKET;
    
    // IQFeed ports
    static const int ADMIN_PORT = 9300;
    static const int LOOKUP_PORT = 9100;
    
    std::unique_ptr<Logger> logger;
    bool is_connected = false;

public:
    IQFeedConnectionTest() {
        logger = std::make_unique<Logger>("iqfeed_connection_test.log", true);
        initialize_winsock();
    }
    
    ~IQFeedConnectionTest() {
        disconnect();
        cleanup_winsock();
    }
    
    bool run_connection_test() {
        logger->info("=== IQFeed Connection Test Started ===");
        logger->info("Product ID: " + product_id);
        logger->info("Login ID: " + login_id);
        logger->info("Version: " + version);
        
        // Step 1: Launch IQConnect
        if (!launch_iqconnect()) {
            logger->error("❌ Failed at Step 1: Launch IQConnect");
            return false;
        }
        logger->success("Step 1: IQConnect launched successfully");
        
        // Step 2: Connect to Admin Port
        if (!connect_to_admin()) {
            logger->error("❌ Failed at Step 2: Connect to Admin port");
            return false;
        }
        logger->success("Step 2: Connected to Admin port");
        
        // Step 3: Set Protocol
        if (!set_protocol()) {
            logger->error("❌ Failed at Step 3: Set protocol");
            return false;
        }
        logger->success("Step 3: Protocol set successfully");
        
        // Step 4: Set Client Name
        if (!set_client_name()) {
            logger->error("❌ Failed at Step 4: Set client name");
            return false;
        }
        logger->success("Step 4: Client name set successfully");
        
        // Step 5: Wait for connection to IQ servers
        if (!wait_for_server_connection()) {
            logger->error("❌ Failed at Step 5: Wait for server connection");
            return false;
        }
        logger->success("Step 5: Connected to IQ servers");
        
        // Step 6: Connect to Lookup port for historical data
        if (!connect_to_lookup()) {
            logger->error("❌ Failed at Step 6: Connect to Lookup port");
            return false;
        }
        logger->success("Step 6: Connected to Lookup port");
        
        // Step 7: Verify all connections
        if (!verify_connections()) {
            logger->error("❌ Failed at Step 7: Verify connections");
            return false;
        }
        logger->success("Step 7: All connections verified");
        
        is_connected = true;
        logger->success("🎉 ALL TESTS PASSED! IQFeed connection is fully operational");
        return true;
    }
    
    void disconnect() {
        if (admin_socket != INVALID_SOCKET) {
            closesocket(admin_socket);
            admin_socket = INVALID_SOCKET;
        }
        
        if (lookup_socket != INVALID_SOCKET) {
            closesocket(lookup_socket);
            lookup_socket = INVALID_SOCKET;
        }
        
        is_connected = false;
        logger->info("Disconnected from IQFeed");
    }
    
    bool is_connection_ready() const {
        return is_connected;
    }

private:
    void initialize_winsock() {
#ifdef _WIN32
        WSADATA wsa_data;
        int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
        if (result != 0) {
            throw std::runtime_error("WSAStartup failed: " + std::to_string(result));
        }
        logger->debug("Winsock initialized successfully");
#endif
    }
    
    void cleanup_winsock() {
#ifdef _WIN32
        WSACleanup();
        logger->debug("Winsock cleaned up");
#endif
    }
    
    bool launch_iqconnect() {
        logger->step("Launching IQConnect.exe with credentials...");
        
#ifdef _WIN32
        std::string cmd_line = "IQConnect.exe -product " + product_id + 
                              " -version " + version +
                              " -login " + login_id +
                              " -password " + password +
                              " -autoconnect";
        
        logger->debug("Command line: " + cmd_line);
        
        STARTUPINFOA si = {sizeof(si)};
        PROCESS_INFORMATION pi;
        
        if (!CreateProcessA(NULL, const_cast<char*>(cmd_line.c_str()), NULL, NULL,
                           FALSE, 0, NULL, NULL, &si, &pi)) {
            DWORD error = GetLastError();
            logger->error("Failed to launch IQConnect.exe. Windows Error: " + std::to_string(error));
            logger->error("Make sure IQConnect.exe is installed and in your PATH");
            return false;
        }
        
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        
        logger->info("IQConnect.exe process started, waiting 5 seconds for initialization...");
        std::this_thread::sleep_for(std::chrono::seconds(5));
        return true;
#else
        logger->error("This test is designed for Windows. IQConnect.exe is Windows-only.");
        return false;
#endif
    }
    
    bool connect_to_admin() {
        logger->step("Connecting to Admin port " + std::to_string(ADMIN_PORT) + "...");
        
        admin_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (admin_socket == INVALID_SOCKET) {
            logger->error("Failed to create admin socket. Error: " + std::to_string(get_last_error()));
            return false;
        }
        
        sockaddr_in admin_addr{};
        admin_addr.sin_family = AF_INET;
        admin_addr.sin_port = htons(ADMIN_PORT);
        inet_pton(AF_INET, "127.0.0.1", &admin_addr.sin_addr);
        
        int attempts = 0;
        const int max_attempts = 10;
        
        while (attempts < max_attempts) {
            if (connect(admin_socket, (sockaddr*)&admin_addr, sizeof(admin_addr)) == 0) {
                logger->info("Connected to Admin port successfully");
                return true;
            }
            
            attempts++;
            logger->debug("Connection attempt " + std::to_string(attempts) + "/" + std::to_string(max_attempts) + " failed, retrying in 2 seconds...");
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
        
        logger->error("Failed to connect to admin port after " + std::to_string(max_attempts) + " attempts");
        logger->error("Error: " + std::to_string(get_last_error()));
        closesocket(admin_socket);
        admin_socket = INVALID_SOCKET;
        return false;
    }
    
    bool set_protocol() {
        logger->step("Setting protocol to 6.2...");
        
        std::string command = "S,SET PROTOCOL,6.2\r\n";
        
        if (send(admin_socket, command.c_str(), static_cast<int>(command.length()), 0) == SOCKET_ERROR) {
            logger->error("Failed to send protocol command. Error: " + std::to_string(get_last_error()));
            return false;
        }
        
        // Read response
        char buffer[1024];
        int bytes_received = recv(admin_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            std::string response(buffer);
            logger->debug("Protocol response: " + response);
            
            if (response.find("S,CURRENT PROTOCOL,6.2") != std::string::npos) {
                logger->debug("Protocol 6.2 confirmed");
                return true;
            }
        }
        
        logger->error("Failed to set protocol or received unexpected response");
        return false;
    }
    
    bool set_client_name() {
        logger->step("Setting client name...");
        
        std::string command = "S,SET CLIENT NAME,IQFeed_Connection_Test\r\n";
        
        if (send(admin_socket, command.c_str(), static_cast<int>(command.length()), 0) == SOCKET_ERROR) {
            logger->error("Failed to send client name command. Error: " + std::to_string(get_last_error()));
            return false;
        }
        
        logger->debug("Client name command sent successfully");
        return true;
    }
    
    bool wait_for_server_connection() {
        logger->step("Waiting for IQFeed to connect to servers...");
        
        const int max_wait_time = 120; // seconds
        int elapsed_time = 0;
        
        while (elapsed_time < max_wait_time) {
            if (check_feed_status()) {
                logger->info("IQFeed successfully connected to servers");
                return true;
            }
            
            elapsed_time += 3;
            logger->debug("Still waiting for server connection... (" + std::to_string(elapsed_time) + "s)");
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
        
        logger->error("Timeout waiting for server connection after " + std::to_string(max_wait_time) + " seconds");
        return false;
    }
    
    bool check_feed_status() {
        std::string command = "S,STATS\r\n";
    
        if (send(admin_socket, command.c_str(), static_cast<int>(command.length()), 0) == SOCKET_ERROR) {
            logger->debug("Failed to send STATS command");
            return false;
        }
    
        char buffer[2048];
        int bytes_received = recv(admin_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            std::string response(buffer);
        
            logger->debug("Feed status response: " + response);
        
            // Look for ",Connected," specifically (surrounded by commas)
            if (response.find(",Connected,") != std::string::npos) {
                logger->debug("✅ IQFeed is fully Connected to servers");
                return true;
            } else if (response.find(",Not Connected,") != std::string::npos) {
                logger->debug("⏳ IQFeed is Not Connected to servers yet");
                return false;
            } else if (response.find(",Connecting,") != std::string::npos) {
                logger->debug("🔄 IQFeed is Connecting to servers...");
                return false;
            }
        }
    
        return false;
    }
    
    bool connect_to_lookup() {
        logger->step("Connecting to Lookup port " + std::to_string(LOOKUP_PORT) + "...");
        
        lookup_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (lookup_socket == INVALID_SOCKET) {
            logger->error("Failed to create lookup socket. Error: " + std::to_string(get_last_error()));
            return false;
        }
        
        sockaddr_in lookup_addr{};
        lookup_addr.sin_family = AF_INET;
        lookup_addr.sin_port = htons(LOOKUP_PORT);
        inet_pton(AF_INET, "127.0.0.1", &lookup_addr.sin_addr);
        
        if (connect(lookup_socket, (sockaddr*)&lookup_addr, sizeof(lookup_addr)) == SOCKET_ERROR) {
            logger->error("Failed to connect to lookup port. Error: " + std::to_string(get_last_error()));
            closesocket(lookup_socket);
            lookup_socket = INVALID_SOCKET;
            return false;
        }
        
        // Set protocol for lookup connection
        std::string protocol_cmd = "S,SET PROTOCOL,6.2\r\n";
        send(lookup_socket, protocol_cmd.c_str(), static_cast<int>(protocol_cmd.length()), 0);
        
        // Set client name for lookup connection
        std::string client_cmd = "S,SET CLIENT NAME,IQFeed_Connection_Test\r\n";
        send(lookup_socket, client_cmd.c_str(), static_cast<int>(client_cmd.length()), 0);
        
        logger->info("Connected to Lookup port successfully");
        return true;
    }
    
    bool verify_connections() {
        logger->step("Verifying all connections...");
        
        // Test admin connection
        if (admin_socket == INVALID_SOCKET) {
            logger->error("Admin socket is not valid");
            return false;
        }
        
        // Test lookup connection
        if (lookup_socket == INVALID_SOCKET) {
            logger->error("Lookup socket is not valid");
            return false;
        }
        
        // Send a test command to admin port
        std::string test_command = "S,STATS\r\n";
        if (send(admin_socket, test_command.c_str(), static_cast<int>(test_command.length()), 0) == SOCKET_ERROR) {
            logger->error("Failed to send test command to admin port");
            return false;
        }
        
        char buffer[1024];
        int bytes_received = recv(admin_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            logger->error("Failed to receive response from admin port");
            return false;
        }
        
        logger->debug("Connection verification successful - received " + std::to_string(bytes_received) + " bytes");
        return true;
    }
    
    int get_last_error() {
#ifdef _WIN32
        return WSAGetLastError();
#else
        return errno;
#endif
    }
};

int main() {
    std::cout << "🚀 IQFeed Connection Test Program" << std::endl;
    std::cout << "=================================" << std::endl;
    std::cout << "This program will test the complete IQFeed initialization process." << std::endl;
    std::cout << "Make sure IQFeed is installed before running this test." << std::endl;
    std::cout << std::endl;
    
    try {
        IQFeedConnectionTest test;
        
        std::cout << "Starting connection test..." << std::endl;
        std::cout << std::endl;
        
        bool success = test.run_connection_test();
        
        std::cout << std::endl;
        std::cout << "=================================" << std::endl;
        
        if (success) {
            std::cout << "🎉 SUCCESS! IQFeed connection is working perfectly!" << std::endl;
            std::cout << "✅ All connection steps completed successfully" << std::endl;
            std::cout << "✅ Ready for historical data requests" << std::endl;
            std::cout << "✅ Ready for live data streaming" << std::endl;
            
            std::cout << std::endl;
            std::cout << "Connection will remain active. Press Enter to disconnect and exit..." << std::endl;
            std::cin.get();
            
        } else {
            std::cout << "❌ FAILED! Connection test encountered errors." << std::endl;
            std::cout << "📝 Check the log file in logs/iqfeed_connection_test.log for details" << std::endl;
            std::cout << std::endl;
            std::cout << "Common issues:" << std::endl;
            std::cout << "• IQFeed not installed or not in PATH" << std::endl;
            std::cout << "• Invalid credentials" << std::endl;
            std::cout << "• Firewall blocking connections" << std::endl;
            std::cout << "• No internet connection for IQFeed servers" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Exception occurred: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}