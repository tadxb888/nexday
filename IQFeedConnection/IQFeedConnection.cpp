#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <memory>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>
#include <iomanip>
#include <algorithm>
#include <cctype>

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

// Structure to hold OHLCV data
struct HistoricalBar {
    std::string date;
    double open;
    double high;
    double low;
    double close;
    int volume;
    int open_interest;
    
    HistoricalBar() : open(0), high(0), low(0), close(0), volume(0), open_interest(0) {}
};

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
    Logger(const std::string& filename = "iqfeed_historical.log", bool enabled = true) 
        : logging_enabled(enabled) {
        if (logging_enabled) {
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
        
        std::cout << log_entry << std::endl;
        
        if (log_file.is_open()) {
            log_file << log_entry << std::endl;
            log_file.flush();
        }
    }
    
    void info(const std::string& message) { 
        log("INFO", message); 
    }
    
    void error(const std::string& message) { 
        log("ERROR", message); 
    }
    
    void debug(const std::string& message) { 
        log("DEBUG", message); 
    }
    
    void success(const std::string& message) { 
        log("SUCCESS", message); 
    }
};

class IQFeedHistoricalData {
private:
    static const int LOOKUP_PORT = 9100;
    std::unique_ptr<Logger> logger;
    bool winsock_initialized = false;

public:
    IQFeedHistoricalData() {
        logger = std::make_unique<Logger>("iqfeed_historical.log", true);
        initialize_winsock();
    }
    
    ~IQFeedHistoricalData() {
        cleanup_winsock();
    }
    
    bool request_historical_data(const std::string& symbol) {
        logger->info("Requesting 5 days of historical data for symbol: " + symbol);
        
        // Create fresh connection for this request
        SOCKET lookup_socket = create_lookup_connection();
        if (lookup_socket == INVALID_SOCKET) {
            logger->error("Failed to connect to IQFeed lookup port");
            return false;
        }
        
        // Send protocol command
        std::string protocol_cmd = "S,SET PROTOCOL,6.2\r\n";
        if (send(lookup_socket, protocol_cmd.c_str(), static_cast<int>(protocol_cmd.length()), 0) == SOCKET_ERROR) {
            logger->error("Failed to send protocol command");
            closesocket(lookup_socket);
            return false;
        }
        
        // Read protocol response
        char buffer[1024];
        int bytes = recv(lookup_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            logger->debug("Protocol response: " + std::string(buffer));
        }
        
        // Send historical data request
        std::string request_id = "HIST_" + symbol;
        std::string command = "HDX," + symbol + ",5,0," + request_id + ",100,1\r\n";
        
        logger->debug("Sending command: " + command);
        
        if (send(lookup_socket, command.c_str(), static_cast<int>(command.length()), 0) == SOCKET_ERROR) {
            logger->error("Failed to send historical data request. Error: " + std::to_string(get_last_error()));
            closesocket(lookup_socket);
            return false;
        }
        
        // Read response
        std::string response = read_full_response(lookup_socket);
        closesocket(lookup_socket);
        
        if (response.empty()) {
            logger->error("No response received for historical data request");
            return false;
        }
        
        logger->debug("Raw response received (" + std::to_string(response.length()) + " characters)");
        
        // Parse and display the historical data
        return parse_and_display_historical_data(response, symbol);
    }

private:
    void initialize_winsock() {
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
    
    void cleanup_winsock() {
#ifdef _WIN32
        if (winsock_initialized) {
            WSACleanup();
            logger->debug("Winsock cleaned up");
        }
#endif
    }
    
    SOCKET create_lookup_connection() {
        logger->debug("Connecting to IQFeed lookup port 9100...");
        
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
        
        logger->debug("Connected to lookup port successfully");
        return lookup_socket;
    }
    
    std::string read_full_response(SOCKET socket) {
        std::string full_response;
        char buffer[4096];
        int attempts = 0;
        const int max_attempts = 40; // 20 seconds timeout
        
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
    
    bool parse_and_display_historical_data(const std::string& response, const std::string& symbol) {
        logger->debug("Parsing historical data response...");
        
        // Check for error messages first
        if (response.find("E,") != std::string::npos) {
            logger->error("Error in historical data response: " + response);
            return false;
        }
        
        // Split response into lines
        std::vector<std::string> lines;
        std::istringstream stream(response);
        std::string line;
        
        while (std::getline(stream, line)) {
            if (!line.empty() && line != "\r" && line.find("!ENDMSG!") == std::string::npos) {
                lines.push_back(line);
            }
        }
        
        std::vector<HistoricalBar> historical_data;
        
        for (const auto& line : lines) {
            // Skip empty lines and system messages
            if (line.empty() || line.find("S,") == 0) {
                continue;
            }
            
            // Parse CSV line: HIST_SYMBOL,LH,Date,High,Low,Open,Close,Volume,OpenInterest
            std::vector<std::string> fields = split_csv(line);
            
            if (fields.size() >= 8) {
                HistoricalBar bar;
                
                try {
                    // Fields: RequestID,LH,Date,High,Low,Open,Close,Volume,OpenInterest
                    bar.date = fields[2];           // Date
                    bar.high = std::stod(fields[3]); // High
                    bar.low = std::stod(fields[4]);  // Low  
                    bar.open = std::stod(fields[5]); // Open
                    bar.close = std::stod(fields[6]); // Close
                    bar.volume = std::stoi(fields[7]); // Volume
                    if (fields.size() > 8) {
                        bar.open_interest = std::stoi(fields[8]); // Open Interest
                    }
                    
                    historical_data.push_back(bar);
                } catch (const std::exception& e) {
                    logger->debug("Failed to parse line: " + line + " - Error: " + e.what());
                    continue;
                }
            }
        }
        
        // Display the results
        display_historical_data(symbol, historical_data);
        
        return !historical_data.empty();
    }
    
    std::vector<std::string> split_csv(const std::string& line) {
        std::vector<std::string> fields;
        std::string field;
        bool in_quotes = false;
        
        for (size_t i = 0; i < line.length(); ++i) {
            char c = line[i];
            
            if (c == '"') {
                in_quotes = !in_quotes;
            } else if (c == ',' && !in_quotes) {
                fields.push_back(field);
                field.clear();
            } else if (c != '\r' && c != '\n') {
                field += c;
            }
        }
        
        if (!field.empty()) {
            fields.push_back(field);
        }
        
        return fields;
    }
    
    void display_historical_data(const std::string& symbol, const std::vector<HistoricalBar>& data) {
        std::cout << "\n" << std::string(80, '=') << std::endl;
        std::cout << "HISTORICAL DATA FOR " << symbol << " (Last 5 Days)" << std::endl;
        std::cout << std::string(80, '=') << std::endl;
        
        if (data.empty()) {
            std::cout << "No historical data found for symbol: " << symbol << std::endl;
            std::cout << "This could mean:" << std::endl;
            std::cout << "  * Invalid symbol" << std::endl;
            std::cout << "  * Symbol not available in your data subscription" << std::endl;
            std::cout << "  * No trading data available for the requested period" << std::endl;
            return;
        }
        
        // Header
        std::cout << std::left;
        std::cout << std::setw(12) << "Date" 
                  << std::setw(10) << "Open"
                  << std::setw(10) << "High"
                  << std::setw(10) << "Low"
                  << std::setw(10) << "Close"
                  << std::setw(12) << "Volume"
                  << std::setw(12) << "Open Int."
                  << std::endl;
        
        std::cout << std::string(80, '-') << std::endl;
        
        // Data rows
        for (const auto& bar : data) {
            std::cout << std::fixed << std::setprecision(2);
            std::cout << std::setw(12) << bar.date
                      << std::setw(10) << bar.open
                      << std::setw(10) << bar.high
                      << std::setw(10) << bar.low
                      << std::setw(10) << bar.close
                      << std::setw(12) << bar.volume
                      << std::setw(12) << bar.open_interest
                      << std::endl;
        }
        
        std::cout << std::string(80, '=') << std::endl;
        std::cout << "Successfully retrieved " << data.size() << " days of historical data" << std::endl;
    }
    
    int get_last_error() {
#ifdef _WIN32
        return WSAGetLastError();
#else
        return errno;
#endif
    }
};

// Function to get user input for symbol
std::string get_symbol_from_user() {
    std::string symbol;
    std::cout << "\nEnter the symbol you want to get historical data for: ";
    std::getline(std::cin, symbol);
    
    // Convert to uppercase
    for (char& c : symbol) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    
    // Remove whitespace manually
    std::string result;
    for (char c : symbol) {
        if (!std::isspace(static_cast<unsigned char>(c))) {
            result += c;
        }
    }
    
    return result;
}

int main() {
    std::cout << "IQFeed Historical Data Program" << std::endl;
    std::cout << "==============================" << std::endl;
    std::cout << std::endl;
    std::cout << "IMPORTANT: Before running this program:" << std::endl;
    std::cout << "1. Launch IQConnect.exe manually" << std::endl;
    std::cout << "2. Login with your credentials (523576 / 56719893)" << std::endl;
    std::cout << "3. Wait for 'Connected' status" << std::endl;
    std::cout << "4. Then use this program to request historical data" << std::endl;
    std::cout << std::endl;
    std::cout << "Press Enter when IQConnect is running and connected..." << std::endl;
    std::cin.get();
    
    try {
        IQFeedHistoricalData iqfeed;
        
        // Interactive loop for requesting historical data
        while (true) {
            std::string symbol = get_symbol_from_user();
            
            if (symbol.empty()) {
                std::cout << "Empty symbol entered. Please try again." << std::endl;
                continue;
            }
            
            if (symbol == "QUIT" || symbol == "EXIT") {
                break;
            }
            
            std::cout << "\nRequesting historical data for: " << symbol << std::endl;
            
            bool success = iqfeed.request_historical_data(symbol);
            
            if (!success) {
                std::cout << "Failed to retrieve historical data for " << symbol << std::endl;
                std::cout << "Try symbols like: AAPL, MSFT, SPY, QQQ, TSLA" << std::endl;
            }
            
            std::cout << "\n" << std::string(50, '=') << std::endl;
            std::cout << "Enter another symbol, or type 'quit' to exit." << std::endl;
        }
        
        std::cout << "\nThank you for using IQFeed Historical Data Program!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception occurred: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}