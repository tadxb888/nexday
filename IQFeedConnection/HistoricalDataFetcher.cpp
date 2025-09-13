#include "HistoricalDataFetcher.h"
#include "IQFeedConnectionManager.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <ctime>

HistoricalDataFetcher::HistoricalDataFetcher(std::shared_ptr<IQFeedConnectionManager> conn_mgr, 
                                           const std::string& period)
    : connection_manager(conn_mgr), period_name(period) {
    logger = std::make_unique<Logger>("iqfeed_" + period + ".log", true);
}

bool HistoricalDataFetcher::fetch_historical_data(const std::string& symbol, int num_bars, 
                                                 std::vector<HistoricalBar>& data) {
    if (!connection_manager || !connection_manager->is_connection_ready()) {
        logger->error("Connection manager not ready");
        return false;
    }
    
    logger->info("Fetching " + std::to_string(num_bars) + " " + period_name + 
                 " bars for symbol: " + symbol);
    
    // Create fresh socket for this request
    SOCKET lookup_socket = connection_manager->create_lookup_socket();
    if (lookup_socket == INVALID_SOCKET) {
        logger->error("Failed to create lookup socket");
        return false;
    }
    
    // Build command based on timeframe
    std::string request_id = "HIST_" + symbol + "_" + period_name;
    std::string command;
    
    // Use different command format based on timeframe
    std::string interval_code = get_interval_code();
    if (interval_code == "DAILY") {
        // Daily data uses HDX command - explicitly exclude partial datapoint
        command = "HDX," + symbol + "," + std::to_string(num_bars) + ",0," + request_id + ",100,0\r\n";
    } else {
        // Intraday data uses HIX command with LabelAtBeginning=0 for end timestamps
        // HIX,Symbol,Interval,MaxDatapoints,DataDirection,RequestID,DatapointsPerSend,IntervalType,LabelAtBeginning
        command = "HIX," + symbol + "," + interval_code + "," + std::to_string(num_bars) + 
                 ",1," + request_id + ",100,s,0\r\n";
    }
    
    logger->debug("Sending command: " + command);
    
    // Send command
    if (!connection_manager->send_command(lookup_socket, command)) {
        connection_manager->close_lookup_socket(lookup_socket);
        return false;
    }
    
    // Read response
    std::string response = connection_manager->read_full_response(lookup_socket);
    connection_manager->close_lookup_socket(lookup_socket);
    
    if (response.empty()) {
        logger->error("No response received");
        return false;
    }
    
    logger->debug("Raw response received (" + std::to_string(response.length()) + " characters)");
    
    // Parse data
    return parse_historical_data(response, symbol, data);
}

bool HistoricalDataFetcher::is_complete_bar(const std::string& datetime_str) const {
    if (get_interval_code() == "DAILY") {
        // For daily data, check if it's not today's date
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        auto tm_now = *std::localtime(&time_t_now);
        
        std::ostringstream today_str;
        today_str << std::put_time(&tm_now, "%Y-%m-%d");
        
        bool is_complete = datetime_str != today_str.str();
        
        // Debug: Log the comparison
        logger->debug("Daily bar date check: " + datetime_str + " vs today " + today_str.str() + 
                     " -> " + (is_complete ? "COMPLETE" : "INCOMPLETE"));
        
        return is_complete;
    } else {
        // For intraday data, parse the timestamp and check if it's at least one full interval ago
        std::istringstream ss(datetime_str);
        std::tm tm = {};
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
        
        if (ss.fail()) {
            logger->debug("Failed to parse datetime: " + datetime_str);
            return false; // If can't parse, consider incomplete to be safe
        }
        
        auto bar_time = std::chrono::system_clock::from_time_t(std::mktime(&tm));
        auto now = std::chrono::system_clock::now();
        auto interval_duration = get_interval_offset();
        
        // A bar is complete if enough time has passed since its end time
        // Add a small buffer (30 seconds) to account for data processing delays
        auto completion_threshold = bar_time + interval_duration + std::chrono::seconds(30);
        
        bool is_complete = now >= completion_threshold;
        
        if (!is_complete) {
            logger->debug("Bar at " + datetime_str + " is incomplete - current bar still in progress");
        }
        
        return is_complete;
    }
}

bool HistoricalDataFetcher::parse_historical_data(const std::string& response, const std::string& symbol, 
                                                 std::vector<HistoricalBar>& data) {
    logger->debug("Parsing historical data response...");
    
    // Check for error messages
    if (response.find("E,") != std::string::npos) {
        logger->error("Error in response: " + response);
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
    
    // Debug: Print the last few lines of raw response to see the order
    logger->debug("Last 5 lines of raw response:");
    for (int i = std::max(0, (int)lines.size() - 5); i < (int)lines.size(); i++) {
        logger->debug("Raw Line " + std::to_string(i) + ": " + lines[i]);
    }
    
    data.clear();
    int incomplete_bars_filtered = 0;
    
    for (const auto& line : lines) {
        // Skip empty lines and system messages
        if (line.empty() || line.find("S,") == 0) {
            continue;
        }
    
        // Parse CSV line
        std::vector<std::string> fields = split_csv(line);
        
        if (fields.size() >= 7) {  // Minimum fields needed
            HistoricalBar bar;
            std::string full_datetime;
            
            try {
                // Different parsing based on data type
                if (get_interval_code() == "DAILY") {
                    // Daily format: RequestID,LH,Date,High,Low,Open,Close,Volume,OpenInterest
                    if (fields.size() >= 8) {
                        bar.date = fields[2];           // Date at position [2] (after LH field)
                        bar.time = "";                  // No time for daily data
                        full_datetime = bar.date;
                        bar.high = std::stod(fields[3]); // High at position [3]
                        bar.low = std::stod(fields[4]);  // Low at position [4]
                        bar.open = std::stod(fields[5]); // Open at position [5]
                        bar.close = std::stod(fields[6]); // Close at position [6]
                        bar.volume = std::stoi(fields[7]); // Volume at position [7]
                        if (fields.size() > 8) {
                            bar.open_interest = std::stoi(fields[8]); // Open Interest at position [8]
                        }
                    }
                } else {
                    // HIX Intraday format: RequestID,LH,DateTime,High,Low,Open,Close,Volume,TotalVolume,IntervalVolume
                    if (fields.size() >= 8) {
                        // Store original timestamp for debugging
                        std::string original_datetime = fields[2];
                        full_datetime = original_datetime;
                        
                        // Since we used LabelAtBeginning=0, timestamps are at interval END
                        // No adjustment needed - use the timestamp as-is
                        size_t space_pos = original_datetime.find(' ');
                        if (space_pos != std::string::npos) {
                            bar.date = original_datetime.substr(0, space_pos);
                            bar.time = original_datetime.substr(space_pos + 1);
                        } else {
                            bar.date = original_datetime;
                            bar.time = "";
                        }
                        
                        bar.high = std::stod(fields[3]);  // High at position [3]
                        bar.low = std::stod(fields[4]);   // Low at position [4]  
                        bar.open = std::stod(fields[5]);  // Open at position [5]
                        bar.close = std::stod(fields[6]); // Close at position [6]
                        bar.volume = std::stoi(fields[7]); // Volume at position [7]
                        bar.open_interest = 0; // Not available for intraday
                        
                        logger->debug("Parsed bar - Datetime: " + original_datetime + 
                                     " | O:" + std::to_string(bar.open) + 
                                     " H:" + std::to_string(bar.high) + 
                                     " L:" + std::to_string(bar.low) + 
                                     " C:" + std::to_string(bar.close));
                    }
                }
                
                // Check if this bar is complete before adding it
                if (is_complete_bar(full_datetime)) {
                    data.push_back(bar);
                    logger->debug("Added complete bar #" + std::to_string(data.size()) + 
                                 ": " + bar.date + " " + bar.time);
                } else {
                    incomplete_bars_filtered++;
                    logger->debug("Filtered incomplete bar: " + full_datetime);
                }
                
            } catch (const std::exception& e) {
                logger->debug("Failed to parse line: " + line + " - Error: " + e.what());
                continue;
            }
        }
    }
    
    // Debug: Show the order of processed data
    logger->debug("First 5 processed bars:");
    for (int i = 0; i < std::min(5, (int)data.size()); i++) {
        const auto& bar = data[i];
        logger->debug("Bar " + std::to_string(i) + ": " + bar.date + " " + bar.time + 
                     " | O:" + std::to_string(bar.open) + " H:" + std::to_string(bar.high) + 
                     " L:" + std::to_string(bar.low) + " C:" + std::to_string(bar.close));
    }
    
    logger->debug("Last 5 processed bars:");
    for (int i = std::max(0, (int)data.size() - 5); i < (int)data.size(); i++) {
        const auto& bar = data[i];
        logger->debug("Bar " + std::to_string(i) + ": " + bar.date + " " + bar.time + 
                     " | O:" + std::to_string(bar.open) + " H:" + std::to_string(bar.high) + 
                     " L:" + std::to_string(bar.low) + " C:" + std::to_string(bar.close));
    }
    
    logger->success("Successfully parsed " + std::to_string(data.size()) + " complete bars" + 
                   (incomplete_bars_filtered > 0 ? 
                    " (filtered " + std::to_string(incomplete_bars_filtered) + " incomplete bars)" : ""));
    return !data.empty();
}

std::vector<std::string> HistoricalDataFetcher::split_csv(const std::string& line) {
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

void HistoricalDataFetcher::display_historical_data(const std::string& symbol, 
                                                   const std::vector<HistoricalBar>& data) {
    std::cout << "\n" << std::string(95, '=') << std::endl;
    std::cout << "HISTORICAL " << period_name << " DATA FOR " << symbol 
              << " (Last " << data.size() << " Complete Bars)" << std::endl;
    std::cout << std::string(95, '=') << std::endl;
    
    if (data.empty()) {
        std::cout << "No complete historical data found for symbol: " << symbol << std::endl;
        return;
    }
    
    // Header - DATE | TIME | OPEN | HIGH | LOW | CLOSE | VOLUME
    std::cout << std::left;
    std::cout << std::setw(12) << "DATE"
              << std::setw(10) << "TIME"
              << std::setw(10) << "OPEN"
              << std::setw(10) << "HIGH"
              << std::setw(10) << "LOW"
              << std::setw(10) << "CLOSE"
              << std::setw(12) << "VOLUME";
    
    // Add Open Interest column for daily data
    if (get_interval_code() == "DAILY") {
        std::cout << std::setw(12) << "OPEN INT.";
    }
    std::cout << std::endl;
    
    std::cout << std::string(95, '-') << std::endl;
    
    // Data rows (show first 10 for newest dates, since data is ordered newest to oldest)
    int end_idx = std::min(10, static_cast<int>(data.size()));
    for (int i = 0; i < end_idx; ++i) {
        const auto& bar = data[i];
        std::cout << std::fixed << std::setprecision(2);
        
        std::cout << std::setw(12) << bar.date
                  << std::setw(10) << bar.time
                  << std::setw(10) << bar.open
                  << std::setw(10) << bar.high
                  << std::setw(10) << bar.low
                  << std::setw(10) << bar.close
                  << std::setw(12) << bar.volume;
        
        if (get_interval_code() == "DAILY") {
            std::cout << std::setw(12) << bar.open_interest;
        }
        std::cout << std::endl;
    }
    
    std::cout << std::string(95, '=') << std::endl;
    std::cout << "Successfully retrieved " + std::to_string(data.size()) + " complete " + period_name + " bars";
    if (data.size() > 10) {
        std::cout << " (showing first 10 - newest dates)";
    }
    std::cout << std::endl;
}