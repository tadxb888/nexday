#ifndef FETCH_SCHEDULER_H
#define FETCH_SCHEDULER_H

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <thread>
#include <atomic>
#include <map>
#include <mutex>  // Add this include
#include "Logger.h"

// Forward declarations
class SimpleDatabaseManager;
class IQFeedConnectionManager;
class HistoricalDataFetcher;
class DailyDataFetcher;
class FifteenMinDataFetcher;
class ThirtyMinDataFetcher;
class OneHourDataFetcher;
class TwoHourDataFetcher;
struct HistoricalBar;

// Scheduling configuration
struct ScheduleConfig {
    std::vector<std::string> symbols = {"QGC#"}; // Default symbols to fetch
    std::string timezone = "America/New_York";   // Eastern Time
    int daily_hour = 19;                         // 7 PM ET
    int daily_minute = 0;                        // 00 minutes
    bool enabled = true;
    
    // Trading days (Sunday=0, Monday=1, ..., Thursday=4)
    std::vector<int> trading_days = {0, 1, 2, 3, 4}; // Sun-Thu
    
    // Number of bars to fetch for each timeframe
    int bars_15min = 100;
    int bars_30min = 100;
    int bars_1hour = 100;
    int bars_2hours = 100;
    int bars_daily = 100;
    
    // Add these missing members that main.cpp expects
    int initial_bars_daily = 100;  // For initial data load
    int recurring_bars = 1;        // For recurring fetches (latest bar only)
};

// Fetch status tracking
struct FetchStatus {
    std::string timeframe;
    std::string symbol;
    std::chrono::system_clock::time_point scheduled_time;
    std::chrono::system_clock::time_point actual_time;
    bool successful;
    int bars_fetched;
    std::string error_message;
    
    FetchStatus() : successful(false), bars_fetched(0) {}
};

class FetchScheduler {
private:
    std::shared_ptr<SimpleDatabaseManager> db_manager_;
    std::shared_ptr<IQFeedConnectionManager> iqfeed_manager_;
    std::unique_ptr<Logger> logger_;
    
    // Data fetchers
    std::unique_ptr<DailyDataFetcher> daily_fetcher_;
    std::unique_ptr<FifteenMinDataFetcher> fifteen_min_fetcher_;
    std::unique_ptr<ThirtyMinDataFetcher> thirty_min_fetcher_;
    std::unique_ptr<OneHourDataFetcher> one_hour_fetcher_;
    std::unique_ptr<TwoHourDataFetcher> two_hour_fetcher_;
    
    ScheduleConfig config_;
    std::atomic<bool> running_;
    std::atomic<bool> shutdown_requested_;
    std::thread scheduler_thread_;
    
    // Status tracking
    std::vector<FetchStatus> fetch_history_;
    mutable std::mutex fetch_history_mutex_;  // Now properly declared
    
public:
    FetchScheduler(std::shared_ptr<SimpleDatabaseManager> db_manager,
                  std::shared_ptr<IQFeedConnectionManager> iqfeed_manager);
    ~FetchScheduler();
    
    // Configuration
    void set_config(const ScheduleConfig& config);
    ScheduleConfig get_config() const;
    void add_symbol(const std::string& symbol);
    void remove_symbol(const std::string& symbol);
    
    // Main control methods
    bool start_scheduler();
    void stop_scheduler();
    bool is_running() const;
    
    // Manual operations
    bool fetch_all_data_now(const std::string& symbol = "");
    bool fetch_daily_data_now(const std::string& symbol = "");
    bool fetch_intraday_data_now(const std::string& timeframe, const std::string& symbol = "");
    
    // Recovery operations
    bool recover_missing_data(const std::chrono::system_clock::time_point& from_date,
                             const std::chrono::system_clock::time_point& to_date = std::chrono::system_clock::now());
    bool check_and_recover_today();
    
    // Status and monitoring
    std::vector<FetchStatus> get_recent_fetch_history(int hours = 24) const;
    void print_status_summary() const;
    void log_fetch_summary() const;
    
private:
    // Core scheduling logic
    void scheduler_main_loop();
    
    // Time utilities
    bool is_trading_day(const std::chrono::system_clock::time_point& time) const;
    std::string format_time(const std::chrono::system_clock::time_point& time) const;
    int get_weekday(const std::chrono::system_clock::time_point& time) const;
    std::chrono::system_clock::time_point get_next_daily_schedule() const;
    
    // Fetch operations - Fixed signatures to match implementation
    bool execute_daily_fetch(const std::string& symbol);
    bool execute_intraday_fetch(const std::string& timeframe, const std::string& symbol);
    
    // Data state checking
    bool is_symbol_initialized_in_db(const std::string& symbol, const std::string& timeframe) const;
    
    // Data persistence
    bool save_historical_bars_to_db(const std::string& symbol, const std::string& timeframe, 
                                   const std::vector<HistoricalBar>& bars);
    
    // Recovery logic
    bool is_data_missing_for_timeframe(const std::string& symbol, const std::string& timeframe,
                                      const std::chrono::system_clock::time_point& expected_time) const;
    
    // Status tracking
    void record_fetch_status(const FetchStatus& status);
    void cleanup_old_fetch_history();
    
    // Error handling
    void handle_fetch_error(const std::string& operation, const std::string& error);
};

#endif // FETCH_SCHEDULER_H