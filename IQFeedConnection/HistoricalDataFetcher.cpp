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
        // HIX with FIXED timestamp labeling to match Time & Sales display
        // HIX: Symbol,Interval,MaxDatapoints,DataDirection,RequestID,DatapointsPerSend,IntervalType,LabelAtBeginning
        // LabelAtBeginning=1 (default) means timestamp represents START of interval
        // This matches Time & Sales display convention where 9:30 = 9:30-9:45 bar
        command = "HIX," + symbol + "," + interval_code + "," + std::to_string(num_bars) + 
                 ",0," + request_id + ",100,s,1\r\n";
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
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        
        // FIXED: Use safe localtime_s instead of localtime
#ifdef _WIN32
        std::tm tm_now;
        if (localtime_s(&tm_now, &time_t_now) != 0) {
            return false; // Error handling
        }
#else
        std::tm tm_now = *std::localtime(&time_t_now);
#endif
        
        std::ostringstream today_str;
        today_str << std::put_time(&tm_now, "%Y-%m-%d");
        
        return datetime_str != today_str.str();
    } else {
        // SIMPLIFIED: With LabelAtBeginning=1, timestamps now represent interval START
        // No need for timezone corrections - timestamp alignment is now consistent with Time & Sales
        std::istringstream ss(datetime_str);
        std::tm tm = {};
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
        
        if (ss.fail()) {
            logger->debug("PARSE_FAIL: " + datetime_str);
            return false;
        }
        
        tm.tm_isdst = -1;
        auto bar_start_time = std::chrono::system_clock::from_time_t(std::mktime(&tm));
        
        // Calculate when this interval would end
        int interval_minutes = 0;
        if (period_name == "15min") interval_minutes = 15;
        else if (period_name == "30min") interval_minutes = 30;
        else if (period_name == "1hour") interval_minutes = 60;
        else if (period_name == "2hours") interval_minutes = 120;
        else if (period_name == "4hours") interval_minutes = 240;
        
        auto bar_end_time = bar_start_time + std::chrono::minutes(interval_minutes);
        auto now = std::chrono::system_clock::now();
        
        // Bar is complete if current time is at least 1 minute past its end time  
        auto minutes_since_end = std::chrono::duration_cast<std::chrono::minutes>(now - bar_end_time);
        bool is_complete = minutes_since_end.count() >= 1;
        
        // Debug logging with corrected logic
        auto bar_end_time_t = std::chrono::system_clock::to_time_t(bar_end_time);
        
        // FIXED: Use safe localtime_s instead of localtime
#ifdef _WIN32
        std::tm bar_end_tm;
        if (localtime_s(&bar_end_tm, &bar_end_time_t) != 0) {
            logger->debug("COMPLETENESS_CHECK: Error getting end time for " + datetime_str);
            return false;
        }
#else
        std::tm bar_end_tm = *std::localtime(&bar_end_time_t);
#endif
        
        std::ostringstream end_str;
        end_str << std::put_time(&bar_end_tm, "%Y-%m-%d %H:%M:%S");
        
        logger->debug("COMPLETENESS_CHECK: BarStart=" + datetime_str + 
                     " | BarEnd=" + end_str.str() + 
                     " | MinutesSinceEnd=" + std::to_string(minutes_since_end.count()) + 
                     " | Result=" + (is_complete ? "COMPLETE" : "INCOMPLETE"));
        
        return is_complete;
    }
}

// FIXED: Remove unused parameter warning by using [[maybe_unused]] or (void)symbol
bool HistoricalDataFetcher::parse_historical_data(const std::string& response, const std::string& symbol, 
                                                 std::vector<HistoricalBar>& data) {
    // Suppress unused parameter warning
    (void)symbol; // FIXED: Explicitly mark parameter as intentionally unused
    
    logger->debug("Parsing historical data response...");
    
    // Check for error messages
    if (response.find("E,") != std::string::npos) {
        logger->error("Error in response: " + response);
        return false;
    }
    
    // Split response into lines
    std::vector<std::string> lines;
    std::istringstream stream(response);
    std::string response_line; // FIXED: Renamed from 'line' to avoid shadowing
    
    while (std::getline(stream, response_line)) {
        if (!response_line.empty() && response_line != "\r" && response_line.find("!ENDMSG!") == std::string::npos) {
            lines.push_back(response_line);
        }
    }
    
    // DEBUG: Show first few raw lines to understand data structure
    logger->debug("First 5 lines of raw response:");
    for (int i = 0; i < std::min(5, (int)lines.size()); i++) {
        logger->debug("Raw Line " + std::to_string(i) + ": " + lines[i]);
    }
    
    // DEBUG: Show last few lines of raw response to see the order
    logger->debug("Last 5 lines of raw response:");
    for (int i = std::max(0, (int)lines.size() - 5); i < (int)lines.size(); i++) {
        logger->debug("Raw Line " + std::to_string(i) + ": " + lines[i]);
    }
    
    data.clear();
    std::vector<HistoricalBar> all_bars; // Store all parsed bars first
    
    // FIXED: Use different variable name to avoid shadowing
    for (const auto& data_line : lines) {
        // Skip empty lines and system messages
        if (data_line.empty() || data_line.find("S,") == 0) {
            continue;
        }
    
        // Parse CSV line
        std::vector<std::string> fields = split_csv(data_line);
        
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
                        // Store original timestamp - now correctly represents interval START
                        std::string original_datetime = fields[2];
                        full_datetime = original_datetime;
                        
                        // With LabelAtBeginning=1, timestamp represents interval START time
                        // This now matches Time & Sales convention exactly
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
                        
                        logger->debug("Parsed bar - StartTime: " + original_datetime + 
                                     " | O:" + std::to_string(bar.open) + 
                                     " H:" + std::to_string(bar.high) + 
                                     " L:" + std::to_string(bar.low) + 
                                     " C:" + std::to_string(bar.close));
                    }
                }
                
                // Store all bars for processing
                all_bars.push_back(bar);
                
            } catch (const std::exception& e) {
                logger->debug("Failed to parse line: " + data_line + " - Error: " + e.what());
                continue;
            }
        }
    }
    
    // DEBUG: Show parsed bars structure
    logger->debug("First 3 parsed bars:");
    for (size_t i = 0; i < std::min(size_t(3), all_bars.size()); i++) {
        const auto& bar = all_bars[i];
        std::string full_datetime = (bar.time.empty()) ? bar.date : bar.date + " " + bar.time;
        logger->debug("ParsedBar[" + std::to_string(i) + "] = " + full_datetime + 
                     " OHLCV: " + std::to_string(bar.open) + "/" + 
                     std::to_string(bar.high) + "/" + std::to_string(bar.low) + "/" + 
                     std::to_string(bar.close) + "/" + std::to_string(bar.volume));
    }
    
    int incomplete_bars_filtered = 0;
    
    if (get_interval_code() == "DAILY") {
        // DAILY DATA: Use raw data as-is - no corrections needed
        // IQFeed daily data is correctly aligned (unlike intraday)
        
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        
        // FIXED: Use safe localtime_s instead of localtime
#ifdef _WIN32
        std::tm tm_now;
        if (localtime_s(&tm_now, &time_t_now) != 0) {
            logger->error("Failed to get current time for daily bar filtering");
            return false;
        }
#else
        std::tm tm_now = *std::localtime(&time_t_now);
#endif
        
        std::ostringstream today_str;
        today_str << std::put_time(&tm_now, "%Y-%m-%d");
        
        for (const auto& bar : all_bars) {
            if (bar.date != today_str.str()) {
                data.push_back(bar);
                // Debug: Show OHLC values in correct order for verification
                logger->debug("Added daily bar: " + bar.date + 
                             " | OHLC: " + std::to_string(bar.open) + "/" + 
                             std::to_string(bar.high) + "/" + std::to_string(bar.low) + "/" + 
                             std::to_string(bar.close) + " Vol:" + std::to_string(bar.volume));
            } else {
                incomplete_bars_filtered++;
                logger->debug("Filtered today's incomplete bar: " + bar.date);
            }
        }
        logger->info("Daily data used as-is (correctly aligned) - " + std::to_string(data.size()) + " complete bars");
    } else {
        // INTRADAY DATA: Use existing working logic (don't change!)
        if (all_bars.size() >= 2) {
            // Create corrected first bar: timestamp from bar[0], OHLCV from bar[1]
            HistoricalBar corrected_first_bar;
            corrected_first_bar.date = all_bars[0].date;   // Correct timestamp
            corrected_first_bar.time = all_bars[1].time;   // Use time from same bar as OHLCV data
            corrected_first_bar.open = all_bars[1].open;   // Correct OHLCV
            corrected_first_bar.high = all_bars[1].high;   // Correct OHLCV  
            corrected_first_bar.low = all_bars[1].low;     // Correct OHLCV
            corrected_first_bar.close = all_bars[1].close; // Correct OHLCV
            corrected_first_bar.volume = all_bars[1].volume; // Correct OHLCV
            corrected_first_bar.open_interest = all_bars[1].open_interest;
            
            std::string corrected_datetime = corrected_first_bar.date + " " + corrected_first_bar.time;
            
            if (is_complete_bar(corrected_datetime)) {
                data.push_back(corrected_first_bar);
                logger->debug("Added corrected intraday bar: " + corrected_first_bar.date + " " + corrected_first_bar.time + 
                             " (timestamp from line 0, OHLCV from line 1)");
            }
            
            // Continue with remaining bars starting from index 2
            for (size_t i = 2; i < all_bars.size(); i++) {
                const auto& bar = all_bars[i];
                std::string full_datetime = bar.date + " " + bar.time;
                
                if (is_complete_bar(full_datetime)) {
                    data.push_back(bar);
                    logger->debug("Added intraday bar #" + std::to_string(data.size()) + 
                                 ": " + bar.date + " " + bar.time);
                } else {
                    incomplete_bars_filtered++;
                }
            }
            logger->info("Applied intraday timestamp/OHLCV correction - using " + std::to_string(data.size()) + " bars");
        }
    }
    
    // Debug: Show the order of final processed data
    logger->debug("First 5 final processed bars:");
    for (int i = 0; i < std::min(5, (int)data.size()); i++) {
        const auto& bar = data[i];
        logger->debug("FinalBar[" + std::to_string(i) + "] = " + bar.date + " " + bar.time + 
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

// ADDED: Helper methods for time formatting in debugging
std::string HistoricalDataFetcher::format_current_time() const {
    auto now = std::chrono::system_clock::now();
    return format_time_point(now);
}

std::string HistoricalDataFetcher::format_time_point(const std::chrono::system_clock::time_point& tp) const {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    
    // FIXED: Use safe localtime_s instead of localtime
#ifdef _WIN32
    std::tm tm;
    if (localtime_s(&tm, &time_t) != 0) {
        return "ERROR_FORMATTING_TIME";
    }
#else
    std::tm tm = *std::localtime(&time_t);
#endif
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
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