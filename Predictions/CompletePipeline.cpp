#include <iostream>
#include <string>
#include <vector>
#include <memory>

// Include your working components
#include "EMACalculator.h"
#include "PredictionPersister.h"
#include "../Database/database_simple.h"
#include "../IQFeedConnection/IQFeedConnectionManager.h"
#include "../IQFeedConnection/DailyDataFetcher.h"
#include "../IQFeedConnection/FifteenMinDataFetcher.h"
#include "../IQFeedConnection/ThirtyMinDataFetcher.h"
#include "../IQFeedConnection/OneHourDataFetcher.h"
#include "../IQFeedConnection/TwoHourDataFetcher.h"
#include "../IQFeedConnection/HistoricalDataFetcher.h"

// ==============================================
// COMPLETE END-TO-END PIPELINE WITH INTRADAY
// ==============================================

class CompletePipeline {
private:
    std::unique_ptr<SimpleDatabaseManager> db_manager;
    std::shared_ptr<IQFeedConnectionManager> iqfeed_manager;
    
    // All timeframe fetchers
    std::unique_ptr<DailyDataFetcher> daily_fetcher;
    std::unique_ptr<FifteenMinDataFetcher> fifteen_min_fetcher;
    std::unique_ptr<ThirtyMinDataFetcher> thirty_min_fetcher;
    std::unique_ptr<OneHourDataFetcher> one_hour_fetcher;
    std::unique_ptr<TwoHourDataFetcher> two_hour_fetcher;
    
    bool is_initialized;
    
public:
    CompletePipeline() : is_initialized(false) {
        std::cout << "=== INITIALIZING COMPLETE PIPELINE WITH INTRADAY ===" << std::endl;
        
        // Step 1: Initialize database connection
        std::cout << "1. Initializing database connection..." << std::endl;
        DatabaseConfig db_config;
        db_config.host = "localhost";
        db_config.port = 5432;
        db_config.database = "nexday_trading";
        db_config.username = "postgres";
        db_config.password = "magical.521";
        
        db_manager = std::make_unique<SimpleDatabaseManager>(db_config);
        
        if (!db_manager->test_connection()) {
            std::cout << "❌ Database connection FAILED!" << std::endl;
            return;
        }
        std::cout << "✅ Database connection established" << std::endl;
        
        // Step 2: Initialize IQFeed connection
        std::cout << "2. Initializing IQFeed connection..." << std::endl;
        iqfeed_manager = std::make_shared<IQFeedConnectionManager>();
        
        if (!iqfeed_manager->initialize_connection()) {
            std::cout << "❌ IQFeed connection FAILED!" << std::endl;
            std::cout << "   Make sure IQConnect is running and logged in" << std::endl;
            return;
        }
        std::cout << "✅ IQFeed connection established" << std::endl;
        
        // Step 3: Initialize ALL data fetchers
        std::cout << "3. Initializing data fetchers..." << std::endl;
        daily_fetcher = std::make_unique<DailyDataFetcher>(iqfeed_manager);
        fifteen_min_fetcher = std::make_unique<FifteenMinDataFetcher>(iqfeed_manager);
        thirty_min_fetcher = std::make_unique<ThirtyMinDataFetcher>(iqfeed_manager);
        one_hour_fetcher = std::make_unique<OneHourDataFetcher>(iqfeed_manager);
        two_hour_fetcher = std::make_unique<TwoHourDataFetcher>(iqfeed_manager);
        std::cout << "✅ All timeframe data fetchers ready" << std::endl;
        
        is_initialized = true;
        std::cout << "🎉 COMPLETE PIPELINE WITH INTRADAY READY!" << std::endl;
    }
    
    bool is_ready() const {
        return is_initialized;
    }
    
    // Helper function to fetch and process intraday data
    // Helper function to fetch and process intraday data
// Helper function to fetch and process intraday data
bool fetch_and_process_intraday_data(const std::string& symbol, 
                                    const std::string& timeframe,
                                    std::vector<HistoricalBar>& bars,
                                    int bars_to_fetch = 100) {
    
    std::cout << "📡 Fetching " << timeframe << " data for " << symbol << "..." << std::endl;
    
    bool fetch_success = false;
    
    if (timeframe == "15min") {
        fetch_success = fifteen_min_fetcher->fetch_historical_data(symbol, bars_to_fetch, bars);
    } else if (timeframe == "30min") {
        fetch_success = thirty_min_fetcher->fetch_historical_data(symbol, bars_to_fetch, bars);
    } else if (timeframe == "1hour") {
        fetch_success = one_hour_fetcher->fetch_historical_data(symbol, bars_to_fetch, bars);
    } else if (timeframe == "2hours") {
        fetch_success = two_hour_fetcher->fetch_historical_data(symbol, bars_to_fetch, bars);
    }
    
    if (!fetch_success) {
        std::cout << "❌ Failed to fetch " << timeframe << " data" << std::endl;
        return false;
    }
    
    std::cout << "✅ Retrieved " << bars.size() << " " << timeframe << " bars" << std::endl;
    
    if (bars.size() < 15) {
        std::cout << "⚠️  Insufficient " << timeframe << " data: " << bars.size() << " bars (need 15+)" << std::endl;
        return false;
    }
    
    // PERSIST THE INTRADAY BARS TO DATABASE
    if (!persist_intraday_bars(symbol, timeframe, bars)) {
        std::cout << "❌ Failed to persist " << timeframe << " bars to database" << std::endl;
        return false;
    }
    
    return true;
}
    
    // PERSIST THE INTRADAY BARS TO DATABASE
    if (!persist_intraday_bars(symbol, timeframe, bars)) {
        std::cout << "❌ Failed to persist " << timeframe << " bars to database" << std::endl;
        return false;
    }
    
    return true;
}

    // Helper function to persist intraday bars using existing database methods
bool persist_intraday_bars(const std::string& symbol, 
                          const std::string& timeframe,
                          const std::vector<HistoricalBar>& bars) {
    
    std::cout << "💾 Persisting " << timeframe << " historical data to database..." << std::endl;
    std::cout << "[DEBUG] Symbol: " << symbol << std::endl;
    std::cout << "[DEBUG] Timeframe: " << timeframe << std::endl;
    std::cout << "[DEBUG] Bars to persist: " << bars.size() << std::endl;
    
    int saved_count = 0;
    int failed_count = 0;
    
    // Show first few bars being processed for debugging
    int debug_count = std::min(3, (int)bars.size());
    for (int i = 0; i < debug_count; i++) {
        std::cout << "[DEBUG] Bar[" << i << "] date: '" << bars[i].date << "' time: '" << bars[i].time << "'" << std::endl;
    }
    
    for (const auto& bar : bars) {
        bool success = false;
        
        // Use your existing working database insertion methods
        if (timeframe == "15min") {
            success = db_manager->insert_historical_data_15min(symbol, bar.date, bar.time,
                bar.open, bar.high, bar.low, bar.close, bar.volume, bar.open_interest);
        } else if (timeframe == "30min") {
            success = db_manager->insert_historical_data_30min(symbol, bar.date, bar.time,
                bar.open, bar.high, bar.low, bar.close, bar.volume, bar.open_interest);
        } else if (timeframe == "1hour") {
            success = db_manager->insert_historical_data_1hour(symbol, bar.date, bar.time,
                bar.open, bar.high, bar.low, bar.close, bar.volume, bar.open_interest);
        } else if (timeframe == "2hours") {
            success = db_manager->insert_historical_data_2hours(symbol, bar.date, bar.time,
                bar.open, bar.high, bar.low, bar.close, bar.volume, bar.open_interest);
        } else {
            std::cout << "[ERROR] Unknown timeframe for database save: " << timeframe << std::endl;
            failed_count++;
            continue;
        }
        
        if (success) {
            saved_count++;
            if (saved_count <= 3) {
                std::cout << "[DEBUG] Successfully saved bar " << saved_count << " for " << bar.date << " " << bar.time << std::endl;
            }
        } else {
            failed_count++;
            if (failed_count <= 3) {
                std::cout << "[ERROR] Failed to save bar: " << bar.date << " " << bar.time << std::endl;
                std::cout << "[ERROR] Database error: " << db_manager->get_last_error() << std::endl;
            }
        }
    }
    
    std::cout << "✅ Database save for " << symbol << " " << timeframe << ": " 
              << saved_count << " saved, " << failed_count << " failed" << std::endl;
    
    return failed_count == 0;
}
    
    // Calculate and save intraday predictions
    bool process_intraday_predictions(const std::string& symbol, 
                                     const std::string& timeframe,
                                     const std::vector<HistoricalBar>& bars) {
        
        std::cout << "🧮 Calculating " << timeframe << " EMA predictions..." << std::endl;
        
        // Extract OHLC price series
        std::vector<double> high_prices, low_prices;
        
        for (const auto& bar : bars) {
            high_prices.push_back(bar.high);
            low_prices.push_back(bar.low);
        }
        
        // Calculate EMA predictions for High/Low (intraday focuses on range)
        double predicted_high = SimpleEMACalculator::calculate_prediction(high_prices);
        double predicted_low = SimpleEMACalculator::calculate_prediction(low_prices);
        
        if (predicted_high == 0.0 || predicted_low == 0.0) {
            std::cout << "❌ " << timeframe << " EMA calculation failed" << std::endl;
            return false;
        }
        
        std::cout << "✅ " << timeframe << " predictions calculated:" << std::endl;
        std::cout << "   High: " << predicted_high << std::endl;
        std::cout << "   Low:  " << predicted_low << std::endl;
        
        // Save to predictions_all_symbols table
        std::string current_time = PredictionPersister::get_current_timestamp();
        std::string target_time = get_next_interval_time(timeframe);
        
        bool high_saved = PredictionPersister::save_prediction_components(
            *db_manager, symbol, timeframe, timeframe + "_high", predicted_high, target_time);
            
        bool low_saved = PredictionPersister::save_prediction_components(
            *db_manager, symbol, timeframe, timeframe + "_low", predicted_low, target_time);
        
        if (high_saved && low_saved) {
            std::cout << "✅ " << timeframe << " predictions saved to database" << std::endl;
            return true;
        } else {
            std::cout << "❌ Failed to save some " << timeframe << " predictions" << std::endl;
            return false;
        }
    }
    
    // Helper to calculate next interval time
    std::string get_next_interval_time(const std::string& timeframe) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto tm = *std::localtime(&time_t);
        
        // Round to next interval based on timeframe
        if (timeframe == "15min") {
            tm.tm_min = ((tm.tm_min / 15) + 1) * 15;
        } else if (timeframe == "30min") {
            tm.tm_min = ((tm.tm_min / 30) + 1) * 30;
        } else if (timeframe == "1hour") {
            tm.tm_hour += 1;
            tm.tm_min = 0;
        } else if (timeframe == "2hours") {
            tm.tm_hour += 2;
            tm.tm_min = 0;
        }
        
        tm.tm_sec = 0;
        mktime(&tm); // Normalize
        
        std::stringstream ss;
        ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
    
    // Execute complete pipeline for a symbol (WITH INTRADAY)
    bool execute_complete_pipeline(const std::string& symbol) {
        std::cout << "\n================================================" << std::endl;
        std::cout << "EXECUTING COMPLETE PIPELINE FOR: " << symbol << std::endl;
        std::cout << "================================================" << std::endl;
        
        if (!is_ready()) {
            std::cout << "❌ Pipeline not initialized properly" << std::endl;
            return false;
        }
        
        bool pipeline_success = true;
        
        // STEP 1: FETCH DAILY DATA (unchanged)
        std::cout << "\n📡 STEP 1: Fetching daily historical data from IQFeed..." << std::endl;
        
        std::vector<HistoricalBar> daily_bars;
        if (!daily_fetcher->fetch_historical_data(symbol, 100, daily_bars)) {
            std::cout << "❌ Failed to fetch daily data from IQFeed" << std::endl;
            return false;
        }
        
        std::cout << "✅ Retrieved " << daily_bars.size() << " daily bars from IQFeed" << std::endl;
        
        if (daily_bars.size() < 15) {
            std::cout << "❌ Insufficient daily data: " << daily_bars.size() << " bars (need 15+)" << std::endl;
            return false;
        }
        
        // STEP 2: PERSIST DAILY DATA (unchanged)
        std::cout << "\n💾 STEP 2: Persisting daily historical data to database..." << std::endl;
        
        int saved_daily_bars = 0;
        for (const auto& bar : daily_bars) {
            if (db_manager->insert_historical_data_daily(symbol, bar.date, 
                bar.open, bar.high, bar.low, bar.close, bar.volume, bar.open_interest)) {
                saved_daily_bars++;
            }
        }
        
        std::cout << "✅ Saved " << saved_daily_bars << "/" << daily_bars.size() << " daily bars to database" << std::endl;
        
        // STEP 3: FETCH AND PROCESS ALL INTRADAY TIMEFRAMES
        std::cout << "\n📡 STEP 3: Fetching and processing intraday data..." << std::endl;

        std::vector<std::string> timeframes = {"15min", "30min", "1hour", "2hours"};
        int successful_intraday = 0;
        int total_intraday_bars_saved = 0;

        for (const auto& timeframe : timeframes) {
            std::vector<HistoricalBar> intraday_bars;
            
            if (fetch_and_process_intraday_data(symbol, timeframe, intraday_bars)) {
                total_intraday_bars_saved += intraday_bars.size();
                if (process_intraday_predictions(symbol, timeframe, intraday_bars)) {
                    successful_intraday++;
                } else {
                    pipeline_success = false;
                }
            } else {
                pipeline_success = false;
            }
        }

        std::cout << "✅ Processed " << successful_intraday << "/" << timeframes.size() << " intraday timeframes" << std::endl;
        std::cout << "✅ Saved total of " << total_intraday_bars_saved << " intraday bars to database" << std::endl;
        
        // STEP 4: CALCULATE AND SAVE DAILY PREDICTIONS
        std::cout << "\n🧮 STEP 4: Calculating daily EMA predictions..." << std::endl;
        
        // Extract OHLC price series from daily data
        std::vector<double> daily_open_prices, daily_high_prices, daily_low_prices, daily_close_prices;
        
        for (const auto& bar : daily_bars) {
            daily_open_prices.push_back(bar.open);
            daily_high_prices.push_back(bar.high);
            daily_low_prices.push_back(bar.low);
            daily_close_prices.push_back(bar.close);
        }
        
        // Calculate daily EMA predictions
        double predicted_open = SimpleEMACalculator::calculate_prediction(daily_open_prices);
        double predicted_high = SimpleEMACalculator::calculate_prediction(daily_high_prices);
        double predicted_low = SimpleEMACalculator::calculate_prediction(daily_low_prices);
        double predicted_close = SimpleEMACalculator::calculate_prediction(daily_close_prices);
        
        if (predicted_open == 0.0 || predicted_high == 0.0 || predicted_low == 0.0 || predicted_close == 0.0) {
            std::cout << "❌ Daily EMA calculation failed" << std::endl;
            return false;
        }
        
        std::cout << "✅ Daily EMA predictions calculated:" << std::endl;
        std::cout << "   Open:  " << predicted_open << std::endl;
        std::cout << "   High:  " << predicted_high << std::endl;
        std::cout << "   Low:   " << predicted_low << std::endl;
        std::cout << "   Close: " << predicted_close << std::endl;
        
        // STEP 5: PERSIST DAILY PREDICTIONS
        std::cout << "\n💾 STEP 5: Persisting daily predictions to database..." << std::endl;
        
        // Create daily prediction structure
        OHLCPrediction daily_prediction;
        daily_prediction.symbol = symbol;
        daily_prediction.predicted_open = predicted_open;
        daily_prediction.predicted_high = predicted_high;
        daily_prediction.predicted_low = predicted_low;
        daily_prediction.predicted_close = predicted_close;
        daily_prediction.target_date = PredictionPersister::get_next_business_day();
        daily_prediction.prediction_time = PredictionPersister::get_current_timestamp();
        daily_prediction.confidence_score = 0.75;
        
        if (!PredictionPersister::save_daily_prediction(*db_manager, daily_prediction)) {
            std::cout << "❌ Failed to save daily prediction to database" << std::endl;
            pipeline_success = false;
        } else {
            std::cout << "✅ Daily prediction saved to database" << std::endl;
        }
        
        // STEP 6: ERROR CALCULATION (if data available)
        std::cout << "\n📈 STEP 6: Calculating prediction errors..." << std::endl;
        
        if (daily_bars.size() >= 20) {
            double last_actual_close = daily_bars[0].close; // Latest bar
            std::string sample_prediction_time = PredictionPersister::get_current_timestamp();
            
            if (PredictionPersister::save_prediction_error(*db_manager, symbol, 
                predicted_close, last_actual_close, sample_prediction_time)) {
                std::cout << "✅ Sample error calculation saved to database" << std::endl;
            } else {
                std::cout << "❌ Failed to save error calculation" << std::endl;
                pipeline_success = false;
            }
        } else {
            std::cout << "⚠️  Insufficient data for error calculation" << std::endl;
        }
        
        // PIPELINE SUMMARY
        std::cout << "\n================================================" << std::endl;
        if (pipeline_success) {
            std::cout << "🎉 COMPLETE PIPELINE: SUCCESS!" << std::endl;
            std::cout << "✅ IQFeed connection established" << std::endl;
            std::cout << "✅ Daily data retrieved (" << daily_bars.size() << " bars)" << std::endl;
            std::cout << "✅ Daily data persisted (" << saved_daily_bars << " bars saved)" << std::endl;
            std::cout << "✅ Intraday data processed (" << successful_intraday << "/" << timeframes.size() << " timeframes)" << std::endl;
            std::cout << "✅ Daily EMA predictions calculated (OHLC)" << std::endl;
            std::cout << "✅ Intraday EMA predictions calculated (High/Low)" << std::endl;
            std::cout << "✅ All predictions persisted to database" << std::endl;
            std::cout << "✅ Error calculation framework active" << std::endl;
        } else {
            std::cout << "⚠️  COMPLETE PIPELINE: PARTIAL SUCCESS" << std::endl;
            std::cout << "Some steps completed successfully, others had issues" << std::endl;
            std::cout << "Check individual step results above for details" << std::endl;
        }
        std::cout << "================================================" << std::endl;
        
        return pipeline_success;
    }
    
    // Run interactive pipeline
    void run_interactive() {
        if (!is_ready()) {
            std::cout << "❌ Pipeline not ready - initialization failed" << std::endl;
            return;
        }
        
        std::cout << "\n================================================" << std::endl;
        std::cout << "NEXDAY COMPLETE PIPELINE - INTRADAY MODE" << std::endl;
        std::cout << "================================================" << std::endl;
        
        while (true) {
            std::cout << "\nEnter symbol to process (or 'quit' to exit): ";
            std::string symbol;
            std::cin >> symbol;
            
            if (symbol == "quit" || symbol == "exit" || symbol == "QUIT" || symbol == "EXIT") {
                std::cout << "Exiting pipeline..." << std::endl;
                break;
            }
            
            if (symbol.empty()) {
                std::cout << "Please enter a valid symbol" << std::endl;
                continue;
            }
            
            // Convert to uppercase
            for (char& c : symbol) {
                c = std::toupper(c);
            }
            
            std::cout << "\nProcessing symbol: " << symbol << std::endl;
            std::cout << "This will execute the complete pipeline:" << std::endl;
            std::cout << "  1. Connect to IQFeed" << std::endl;
            std::cout << "  2. Retrieve daily historical data" << std::endl;
            std::cout << "  3. Retrieve intraday data (15min, 30min, 1hour, 2hours)" << std::endl;
            std::cout << "  4. Calculate daily EMA predictions (OHLC)" << std::endl;
            std::cout << "  5. Calculate intraday EMA predictions (High/Low)" << std::endl;
            std::cout << "  6. Persist all predictions" << std::endl;
            std::cout << "  7. Calculate and persist errors" << std::endl;
            
            std::cout << "\nProceed? (y/n): ";
            std::string confirm;
            std::cin >> confirm;
            
            if (confirm == "y" || confirm == "Y" || confirm == "yes" || confirm == "YES") {
                execute_complete_pipeline(symbol);
            } else {
                std::cout << "Skipping " << symbol << std::endl;
            }
            
            std::cout << "\n" << std::string(50, '=') << std::endl;
        }
    }
};

// ==============================================
// MAIN FUNCTION - SINGLE EXECUTION ENTRY POINT
// ==============================================

int main() {
    std::cout << "================================================" << std::endl;
    std::cout << "NEXDAY MARKETS - COMPLETE PIPELINE WITH INTRADAY" << std::endl;
    std::cout << "================================================" << std::endl;
    std::cout << "One execution runs it all:" << std::endl;
    std::cout << "• Connect to IQFeed" << std::endl;
    std::cout << "• Fetch daily + intraday data (15m, 30m, 1h, 2h)" << std::endl;
    std::cout << "• Calculate timeframe-specific EMA predictions" << std::endl;
    std::cout << "• Persist all predictions to database" << std::endl;
    std::cout << "• Calculate and persist errors" << std::endl;
    std::cout << "================================================" << std::endl;
    
    std::cout << "\nIMPORTANT: Make sure IQConnect is running and logged in!" << std::endl;
    std::cout << "Press Enter when ready..." << std::endl;
    std::cin.get();
    
    try {
        // Create and run complete pipeline
        CompletePipeline pipeline;
        
        if (pipeline.is_ready()) {
            pipeline.run_interactive();
        } else {
            std::cout << "❌ Pipeline initialization failed" << std::endl;
            std::cout << "Check that:" << std::endl;
            std::cout << "  • PostgreSQL is running" << std::endl;
            std::cout << "  • Database 'nexday_trading' exists" << std::endl;
            std::cout << "  • IQConnect is running and logged in" << std::endl;
            return 1;
        }
        
    } catch (const std::exception& e) {
        std::cout << "❌ Exception occurred: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "\nThank you for using Nexday Complete Pipeline!" << std::endl;
    return 0;
}