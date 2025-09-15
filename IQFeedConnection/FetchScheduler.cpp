#include "FetchScheduler.h"
#include "database_simple.h"
#include "IQFeedConnectionManager.h"
#include "DailyDataFetcher.h"
#include "FifteenMinDataFetcher.h"
#include "ThirtyMinDataFetcher.h"
#include "OneHourDataFetcher.h"
#include "TwoHourDataFetcher.h"
#include "HistoricalDataFetcher.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <mutex>        // Add this include

FetchScheduler::FetchScheduler(std::shared_ptr<SimpleDatabaseManager> db_manager,
                              std::shared_ptr<IQFeedConnectionManager> iqfeed_manager)
    : db_manager_(db_manager), iqfeed_manager_(iqfeed_manager), running_(false), shutdown_requested_(false) {
    
    logger_ = std::make_unique<Logger>("fetch_scheduler.log", true);
    
    // Initialize fetchers
    daily_fetcher_ = std::make_unique<DailyDataFetcher>(iqfeed_manager_);
    fifteen_min_fetcher_ = std::make_unique<FifteenMinDataFetcher>(iqfeed_manager_);
    thirty_min_fetcher_ = std::make_unique<ThirtyMinDataFetcher>(iqfeed_manager_);
    one_hour_fetcher_ = std::make_unique<OneHourDataFetcher>(iqfeed_manager_);
    two_hour_fetcher_ = std::make_unique<TwoHourDataFetcher>(iqfeed_manager_);
    
    logger_->info("FetchScheduler initialized");
}

FetchScheduler::~FetchScheduler() {
    stop_scheduler();
}

// ==============================================
// CONFIGURATION METHODS
// ==============================================

void FetchScheduler::set_config(const ScheduleConfig& config) {
    config_ = config;
    logger_->info("Configuration updated. Symbols: " + std::to_string(config_.symbols.size()) + 
                 ", Trading days: " + std::to_string(config_.trading_days.size()));
}

ScheduleConfig FetchScheduler::get_config() const {
    return config_;
}

void FetchScheduler::add_symbol(const std::string& symbol) {
    auto it = std::find(config_.symbols.begin(), config_.symbols.end(), symbol);
    if (it == config_.symbols.end()) {
        config_.symbols.push_back(symbol);
        logger_->info("Added symbol: " + symbol);
    }
}

void FetchScheduler::remove_symbol(const std::string& symbol) {
    auto it = std::find(config_.symbols.begin(), config_.symbols.end(), symbol);
    if (it != config_.symbols.end()) {
        config_.symbols.erase(it);
        logger_->info("Removed symbol: " + symbol);
    }
}

// ==============================================
// MAIN CONTROL METHODS
// ==============================================

bool FetchScheduler::start_scheduler() {
    if (running_) {
        logger_->info("Scheduler already running");
        return true;
    }
    
    if (!iqfeed_manager_ || !iqfeed_manager_->is_connection_ready()) {
        logger_->error("IQFeed connection not ready");
        return false;
    }
    
    if (!db_manager_ || !db_manager_->is_connected()) {
        logger_->error("Database connection not ready");
        return false;
    }
    
    running_ = true;
    shutdown_requested_ = false;
    
    // Start scheduler thread
    scheduler_thread_ = std::thread(&FetchScheduler::scheduler_main_loop, this);
    
    logger_->success("FetchScheduler started successfully");
    std::cout << "=== FETCH SCHEDULER STARTED ===" << std::endl;
    std::cout << "Monitoring " << config_.symbols.size() << " symbols" << std::endl;
    std::cout << "Next daily schedule: " << format_time(get_next_daily_schedule()) << std::endl;
    
    return true;
}

void FetchScheduler::stop_scheduler() {
    if (!running_) {
        return;
    }
    
    logger_->info("Stopping scheduler...");
    shutdown_requested_ = true;
    running_ = false;
    
    if (scheduler_thread_.joinable()) {
        scheduler_thread_.join();
    }
    
    logger_->success("FetchScheduler stopped");
    std::cout << "=== FETCH SCHEDULER STOPPED ===" << std::endl;
}

bool FetchScheduler::is_running() const {
    return running_;
}

// ==============================================
// MANUAL OPERATIONS
// ==============================================

bool FetchScheduler::fetch_all_data_now(const std::string& symbol) {
    std::vector<std::string> symbols_to_fetch;
    if (symbol.empty()) {
        symbols_to_fetch = config_.symbols;
    } else {
        symbols_to_fetch = {symbol};
    }
    
    logger_->info("Manual fetch all data initiated for " + std::to_string(symbols_to_fetch.size()) + " symbols");
    
    bool overall_success = true;
    
    for (const auto& sym : symbols_to_fetch) {
        // Fetch daily data
        if (!execute_daily_fetch(sym)) {
            overall_success = false;
        }
        
        // Fetch all intraday timeframes
        std::vector<std::string> timeframes = {"15min", "30min", "1hour", "2hours"};
        for (const auto& tf : timeframes) {
            if (!execute_intraday_fetch(tf, sym)) {
                overall_success = false;
            }
        }
    }
    
    return overall_success;
}

bool FetchScheduler::fetch_daily_data_now(const std::string& symbol) {
    std::vector<std::string> symbols_to_fetch;
    if (symbol.empty()) {
        symbols_to_fetch = config_.symbols;
    } else {
        symbols_to_fetch = {symbol};
    }
    
    bool success = true;
    for (const auto& sym : symbols_to_fetch) {
        if (!execute_daily_fetch(sym)) {
            success = false;
        }
    }
    
    return success;
}

bool FetchScheduler::fetch_intraday_data_now(const std::string& timeframe, const std::string& symbol) {
    std::vector<std::string> symbols_to_fetch;
    if (symbol.empty()) {
        symbols_to_fetch = config_.symbols;
    } else {
        symbols_to_fetch = {symbol};
    }
    
    bool success = true;
    for (const auto& sym : symbols_to_fetch) {
        if (!execute_intraday_fetch(timeframe, sym)) {
            success = false;
        }
    }
    
    return success;
}

// ==============================================
// CORE SCHEDULING LOGIC
// ==============================================

void FetchScheduler::scheduler_main_loop() {
    logger_->info("Scheduler main loop started");
    
    // Check for missing data on startup
    check_and_recover_today();
    
    auto last_daily_check = std::chrono::system_clock::now() - std::chrono::hours(25); // Force first check
    auto last_15min_fetch = std::chrono::system_clock::now() - std::chrono::minutes(16);
    auto last_30min_fetch = std::chrono::system_clock::now() - std::chrono::minutes(31);
    auto last_1hour_fetch = std::chrono::system_clock::now() - std::chrono::hours(2);
    auto last_2hour_fetch = std::chrono::system_clock::now() - std::chrono::hours(3);
    
    while (!shutdown_requested_) {
        try {
            auto now = std::chrono::system_clock::now();
            
            // Check if it's a trading day and time
            if (is_trading_day(now)) {
                auto current_time = std::chrono::system_clock::to_time_t(now);
                auto tm_now = *std::localtime(&current_time);
                
                bool is_trading_hour = (tm_now.tm_hour == config_.daily_hour && 
                                      tm_now.tm_min >= config_.daily_minute);
                
                if (is_trading_hour) {
                    // Daily fetch check (runs once per day)
                    if (now - last_daily_check >= std::chrono::hours(24)) {
                        logger_->info("Executing scheduled daily fetch");
                        
                        for (const auto& symbol : config_.symbols) {
                            // Check if symbol needs initialization
                            bool needs_initial = !is_symbol_initialized_in_db(symbol, "daily");
                            execute_daily_fetch(symbol);
                        }
                        last_daily_check = now;
                    }
                    
                    // Intraday fetch checks (recurring - fetch only latest bar)
                    if (now - last_15min_fetch >= std::chrono::minutes(15)) {
                        for (const auto& symbol : config_.symbols) {
                            bool needs_initial = !is_symbol_initialized_in_db(symbol, "15min");
                            execute_intraday_fetch("15min", symbol);
                        }
                        last_15min_fetch = now;
                    }
                    
                    if (now - last_30min_fetch >= std::chrono::minutes(30)) {
                        for (const auto& symbol : config_.symbols) {
                            bool needs_initial = !is_symbol_initialized_in_db(symbol, "30min");
                            execute_intraday_fetch("30min", symbol);
                        }
                        last_30min_fetch = now;
                    }
                    
                    if (now - last_1hour_fetch >= std::chrono::hours(1)) {
                        for (const auto& symbol : config_.symbols) {
                            bool needs_initial = !is_symbol_initialized_in_db(symbol, "1hour");
                            execute_intraday_fetch("1hour", symbol);
                        }
                        last_1hour_fetch = now;
                    }
                    
                    if (now - last_2hour_fetch >= std::chrono::hours(2)) {
                        for (const auto& symbol : config_.symbols) {
                            bool needs_initial = !is_symbol_initialized_in_db(symbol, "2hours");
                            execute_intraday_fetch("2hours", symbol);
                        }
                        last_2hour_fetch = now;
                    }
                }
            }
            
            // Cleanup old fetch history periodically
            cleanup_old_fetch_history();
            
            // Sleep for 1 minute before next check
            std::this_thread::sleep_for(std::chrono::minutes(1));
            
        } catch (const std::exception& e) {
            logger_->error("Exception in scheduler loop: " + std::string(e.what()));
            std::this_thread::sleep_for(std::chrono::minutes(1));
        }
    }
    
    logger_->info("Scheduler main loop ended");
}

// ==============================================
// FETCH EXECUTION
// ==============================================

bool FetchScheduler::execute_daily_fetch(const std::string& symbol) {
    FetchStatus status;
    status.timeframe = "daily";
    status.symbol = symbol;
    status.scheduled_time = std::chrono::system_clock::now();
    status.actual_time = std::chrono::system_clock::now();
    
    try {
        std::vector<HistoricalBar> bars;
        if (daily_fetcher_->fetch_historical_data(symbol, config_.bars_daily, bars)) {
            if (save_historical_bars_to_db(symbol, "daily", bars)) {
                status.successful = true;
                status.bars_fetched = bars.size();
                
                logger_->success("Daily fetch completed for " + symbol + ": " + 
                               std::to_string(bars.size()) + " bars");
                
                // Log sample of fetched data
                if (!bars.empty()) {
                    const auto& latest = bars[0]; // First bar is latest
                    logger_->info("Latest daily bar: " + latest.date + " OHLC: " +
                                 std::to_string(latest.open) + "/" + std::to_string(latest.high) + "/" +
                                 std::to_string(latest.low) + "/" + std::to_string(latest.close));
                }
            } else {
                status.successful = false;
                status.error_message = "Database save failed";
                logger_->error("Failed to save daily data for " + symbol + " to database");
            }
        } else {
            status.successful = false;
            status.error_message = "IQFeed fetch failed";
            logger_->error("Failed to fetch daily data for " + symbol + " from IQFeed");
        }
        
    } catch (const std::exception& e) {
        status.successful = false;
        status.error_message = e.what();
        handle_fetch_error("daily fetch for " + symbol, e.what());
    }
    
    record_fetch_status(status);
    return status.successful;
}

bool FetchScheduler::execute_intraday_fetch(const std::string& timeframe, const std::string& symbol) {
    FetchStatus status;
    status.timeframe = timeframe;
    status.symbol = symbol;
    status.scheduled_time = std::chrono::system_clock::now();
    status.actual_time = std::chrono::system_clock::now();
    
    try {
        std::vector<HistoricalBar> bars;  // Declare bars variable here
        int num_bars = 100; // default
        bool fetch_success = false;  // Add missing semicolon
        
        // Use direct method calls on the specific fetcher objects to avoid casting issues
        if (timeframe == "15min") {
            num_bars = config_.bars_15min;
            fetch_success = fifteen_min_fetcher_->fetch_historical_data(symbol, num_bars, bars);
        } else if (timeframe == "30min") {
            num_bars = config_.bars_30min;
            fetch_success = thirty_min_fetcher_->fetch_historical_data(symbol, num_bars, bars);
        } else if (timeframe == "1hour") {
            num_bars = config_.bars_1hour;
            fetch_success = one_hour_fetcher_->fetch_historical_data(symbol, num_bars, bars);
        } else if (timeframe == "2hours") {
            num_bars = config_.bars_2hours;
            fetch_success = two_hour_fetcher_->fetch_historical_data(symbol, num_bars, bars);
        } else {
            status.successful = false;
            status.error_message = "Unknown timeframe: " + timeframe;
            record_fetch_status(status);
            return false;
        }
        
        if (fetch_success) {
            if (save_historical_bars_to_db(symbol, timeframe, bars)) {
                status.successful = true;
                status.bars_fetched = bars.size();
                
                logger_->success(timeframe + " fetch completed for " + symbol + ": " + 
                               std::to_string(bars.size()) + " bars");
                
                // Log sample of fetched data
                if (!bars.empty()) {
                    const auto& latest = bars[0]; // First bar is latest
                    logger_->info("Latest " + timeframe + " bar: " + latest.date + " " + latest.time + 
                                 " OHLC: " + std::to_string(latest.open) + "/" + std::to_string(latest.high) + "/" +
                                 std::to_string(latest.low) + "/" + std::to_string(latest.close));
                }
            } else {
                status.successful = false;
                status.error_message = "Database save failed";
                logger_->error("Failed to save " + timeframe + " data for " + symbol + " to database");
            }
        } else {
            status.successful = false;
            status.error_message = "IQFeed fetch failed";
            logger_->error("Failed to fetch " + timeframe + " data for " + symbol + " from IQFeed");
        }
        
    } catch (const std::exception& e) {
        status.successful = false;
        status.error_message = e.what();
        handle_fetch_error(timeframe + " fetch for " + symbol, e.what());
    }
    
    record_fetch_status(status);
    return status.successful;
}

// ==============================================
// DATA PERSISTENCE
// ==============================================

bool FetchScheduler::save_historical_bars_to_db(const std::string& symbol, const std::string& timeframe, 
                                               const std::vector<HistoricalBar>& bars) {
    if (bars.empty()) {
        logger_->debug("No bars to save for " + symbol + " " + timeframe);
        return true;
    }
    
    int saved_count = 0;
    int failed_count = 0;
    
    for (const auto& bar : bars) {
        bool success = false;
        
        // Use the existing SimpleDatabaseManager method for now
        // Note: You'll need to extend SimpleDatabaseManager to have specific methods for each timeframe
        success = db_manager_->insert_historical_data(symbol, bar.date, 
            bar.open, bar.high, bar.low, bar.close, bar.volume);
        
        if (success) {
            saved_count++;
        } else {
            failed_count++;
        }
    }
    
    logger_->info("Database save for " + symbol + " " + timeframe + ": " + 
                 std::to_string(saved_count) + " saved, " + std::to_string(failed_count) + " failed");
    
    return failed_count == 0;
}

// ==============================================
// TIME UTILITIES
// ==============================================

bool FetchScheduler::is_trading_day(const std::chrono::system_clock::time_point& time) const {
    int weekday = get_weekday(time);
    return std::find(config_.trading_days.begin(), config_.trading_days.end(), weekday) != config_.trading_days.end();
}

int FetchScheduler::get_weekday(const std::chrono::system_clock::time_point& time) const {
    auto time_t = std::chrono::system_clock::to_time_t(time);
    auto tm = *std::localtime(&time_t);
    return tm.tm_wday; // 0=Sunday, 1=Monday, etc.
}

std::chrono::system_clock::time_point FetchScheduler::get_next_daily_schedule() const {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto tm_now = *std::localtime(&time_t_now);
    
    // Find next trading day at 7 PM ET
    for (int days_ahead = 0; days_ahead <= 7; ++days_ahead) {
        auto candidate = now + std::chrono::hours(24 * days_ahead);
        if (is_trading_day(candidate)) {
            auto candidate_time_t = std::chrono::system_clock::to_time_t(candidate);
            auto tm_candidate = *std::localtime(&candidate_time_t);
            
            tm_candidate.tm_hour = config_.daily_hour;
            tm_candidate.tm_min = config_.daily_minute;
            tm_candidate.tm_sec = 0;
            
            auto scheduled = std::chrono::system_clock::from_time_t(std::mktime(&tm_candidate));
            
            if (scheduled > now) {
                return scheduled;
            }
        }
    }
    
    return now + std::chrono::hours(24); // Fallback
}

std::string FetchScheduler::format_time(const std::chrono::system_clock::time_point& time) const {
    auto time_t = std::chrono::system_clock::to_time_t(time);
    auto tm = *std::localtime(&time_t);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

// ==============================================
// RECOVERY OPERATIONS
// ==============================================

bool FetchScheduler::check_and_recover_today() {
    logger_->info("Checking for missing data and initiating recovery...");
    
    auto now = std::chrono::system_clock::now();
    auto start_of_day = now - std::chrono::hours(24);
    
    return recover_missing_data(start_of_day, now);
}

bool FetchScheduler::recover_missing_data(const std::chrono::system_clock::time_point& from_date,
                                        const std::chrono::system_clock::time_point& to_date) {
    logger_->info("Recovery operation initiated from " + format_time(from_date) + " to " + format_time(to_date));
    
    bool recovery_success = true;
    
    for (const auto& symbol : config_.symbols) {
        // Check each timeframe for missing data
        std::vector<std::string> timeframes = {"daily", "15min", "30min", "1hour", "2hours"};
        
        for (const auto& timeframe : timeframes) {
            if (is_data_missing_for_timeframe(symbol, timeframe, from_date)) {
                logger_->info("Missing data detected for " + symbol + " " + timeframe + " - recovering");
                
                if (timeframe == "daily") {
                    if (!execute_daily_fetch(symbol)) {
                        recovery_success = false;
                    }
                } else {
                    if (!execute_intraday_fetch(timeframe, symbol)) {
                        recovery_success = false;
                    }
                }
            }
        }
    }
    
    return recovery_success;
}

bool FetchScheduler::is_data_missing_for_timeframe(const std::string& symbol, const std::string& timeframe,
                                                  const std::chrono::system_clock::time_point& expected_time) const {
    // This is a simplified check - in a real implementation, you'd query the database
    // to check if data exists for the expected time period
    return true; // For now, always assume data might be missing
}

// ==============================================
// STATUS AND MONITORING
// ==============================================

void FetchScheduler::record_fetch_status(const FetchStatus& status) {
    std::lock_guard<std::mutex> lock(fetch_history_mutex_);
    fetch_history_.push_back(status);
}

void FetchScheduler::cleanup_old_fetch_history() {
    std::lock_guard<std::mutex> lock(fetch_history_mutex_);
    auto cutoff = std::chrono::system_clock::now() - std::chrono::hours(168); // Keep 1 week
    
    fetch_history_.erase(
        std::remove_if(fetch_history_.begin(), fetch_history_.end(),
                      [cutoff](const FetchStatus& status) {
                          return status.actual_time < cutoff;
                      }),
        fetch_history_.end());
}

std::vector<FetchStatus> FetchScheduler::get_recent_fetch_history(int hours) const {
    std::lock_guard<std::mutex> lock(fetch_history_mutex_);
    
    auto cutoff = std::chrono::system_clock::now() - std::chrono::hours(hours);
    std::vector<FetchStatus> recent;
    
    for (const auto& status : fetch_history_) {
        if (status.actual_time >= cutoff) {
            recent.push_back(status);
        }
    }
    
    return recent;
}

void FetchScheduler::print_status_summary() const {
    auto recent = get_recent_fetch_history(24);
    
    std::cout << "\n=== FETCH SCHEDULER STATUS (Last 24 Hours) ===" << std::endl;
    std::cout << "Total fetches: " << recent.size() << std::endl;
    
    int successful = 0;
    int failed = 0;
    
    for (const auto& status : recent) {
        if (status.successful) {
            successful++;
        } else {
            failed++;
        }
    }
    
    std::cout << "Successful: " << successful << std::endl;
    std::cout << "Failed: " << failed << std::endl;
    std::cout << "Success rate: " << (recent.empty() ? 0 : (successful * 100 / recent.size())) << "%" << std::endl;
    std::cout << "Next scheduled fetch: " << format_time(get_next_daily_schedule()) << std::endl;
    std::cout << "===============================================" << std::endl;
}

void FetchScheduler::log_fetch_summary() const {
    auto recent = get_recent_fetch_history(1);
    
    if (!recent.empty()) {
        logger_->info("=== Recent Fetch Summary ===");
        for (const auto& status : recent) {
            std::string status_str = status.successful ? "SUCCESS" : "FAILED";
            logger_->info(status.symbol + " " + status.timeframe + ": " + status_str + 
                         " (" + std::to_string(status.bars_fetched) + " bars)");
        }
    }
}

// ==============================================
// ERROR HANDLING
// ==============================================

void FetchScheduler::handle_fetch_error(const std::string& operation, const std::string& error) {
    logger_->error("Fetch error in " + operation + ": " + error);
    // Could also log to database error tables here
}

// ==============================================
// HELPER METHODS
// ==============================================

bool FetchScheduler::is_symbol_initialized_in_db(const std::string& symbol, const std::string& timeframe) const {
    // This is a placeholder - you'd implement actual database checking logic here
    // For now, assume all symbols need initialization
    return false;
}