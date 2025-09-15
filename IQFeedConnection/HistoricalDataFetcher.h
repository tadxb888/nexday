#ifndef HISTORICAL_DATA_FETCHER_H
#define HISTORICAL_DATA_FETCHER_H

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include "Logger.h"

class IQFeedConnectionManager;

// Structure to hold OHLCV data
struct HistoricalBar {
    std::string date;        // YYYY-MM-DD
    std::string time;        // HH:MM:SS
    double open;
    double high;
    double low;
    double close;
    int volume;
    int open_interest;
    
    HistoricalBar() : open(0), high(0), low(0), close(0), volume(0), open_interest(0) {}
};

class HistoricalDataFetcher {
protected:
    std::shared_ptr<IQFeedConnectionManager> connection_manager;
    std::unique_ptr<Logger> logger;
    std::string period_name;
    
    // Pure virtual method to get the IQFeed interval code
    virtual std::string get_interval_code() const = 0;
    
    // Pure virtual method to get interval offset for timestamp correction
    virtual std::chrono::seconds get_interval_offset() const = 0;
    
    // Method to check if a bar is complete (not the current incomplete bar)
    bool is_complete_bar(const std::string& datetime_str) const;
    
    // Data parsing methods
    std::vector<std::string> split_csv(const std::string& line);
    bool parse_historical_data(const std::string& response, const std::string& symbol, 
                              std::vector<HistoricalBar>& data);
    
    // Helper methods for time formatting (for debugging) - THESE WERE MISSING
    std::string format_current_time() const;
    std::string format_time_point(const std::chrono::system_clock::time_point& tp) const;

public:
    HistoricalDataFetcher(std::shared_ptr<IQFeedConnectionManager> conn_mgr, 
                         const std::string& period);
    virtual ~HistoricalDataFetcher() = default;
    
    // Main method to fetch historical data
    bool fetch_historical_data(const std::string& symbol, int num_bars, 
                              std::vector<HistoricalBar>& data);
    
    // Display method
    void display_historical_data(const std::string& symbol, 
                                const std::vector<HistoricalBar>& data);
    
    const std::string& get_period_name() const { return period_name; }
};

#endif // HISTORICAL_DATA_FETCHER_H