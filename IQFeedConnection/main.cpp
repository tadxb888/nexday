#include <iostream>
#include <memory>
#include <string>
#include <chrono>
#include <thread>
#include <iomanip>  // Add this for std::setfill and std::setw

#include "IQFeedConnectionManager.h"
#include "FetchScheduler.h"
#include "database_simple.h"
#include "DailyDataFetcher.h"
#include "FifteenMinDataFetcher.h"
#include "ThirtyMinDataFetcher.h"
#include "OneHourDataFetcher.h"
#include "TwoHourDataFetcher.h"

int main() {
    std::cout << "\n==============================================\n";
    std::cout << "   NEXDAY MARKETS PREDICTIONS SYSTEM\n";
    std::cout << "   Scheduled Data Collection Service\n";
    std::cout << "==============================================\n";
    
    try {
        // Initialize database connection
        std::cout << "1. Initializing database connection..." << std::endl;
        DatabaseConfig db_config;
        db_config.host = "localhost";
        db_config.port = 5432;
        db_config.database = "nexday_trading";
        db_config.username = "nexday_user";
        db_config.password = "nexday_secure_password_2025";
        
        auto db_manager = std::make_shared<SimpleDatabaseManager>(db_config);
        
        if (!db_manager->test_connection()) {
            std::cerr << "Failed to connect to database!" << std::endl;
            return 1;
        }
        
        // Initialize IQFeed connection
        std::cout << "2. Initializing IQFeed connection..." << std::endl;
        auto connection_manager = std::make_shared<IQFeedConnectionManager>();
        
        if (!connection_manager->initialize_connection()) {
            std::cerr << "Failed to initialize IQFeed connection" << std::endl;
            return 1;
        }
        
        std::cout << "IQFeed connection established successfully\n" << std::endl;
        
        // Create and configure scheduler
        std::cout << "3. Creating fetch scheduler..." << std::endl;
        FetchScheduler scheduler(db_manager, connection_manager);
        
        // Configure scheduling parameters
        ScheduleConfig config;
        config.symbols = {"QGC#"}; // Gold rolling contract futures - ONLY ONE SYMBOL FOR TESTING
        config.daily_hour = 19;  // 7 PM ET
        config.daily_minute = 0; // 00 minutes
        config.enabled = true;
        config.trading_days = {0, 1, 2, 3, 4}; // Sunday through Thursday
        config.bars_15min = 100;
        config.bars_30min = 100;
        config.bars_1hour = 100;
        config.bars_2hours = 100;
        config.bars_daily = 100;
        config.initial_bars_daily = 100;  // Use the correct member name
        config.recurring_bars = 1;        // Use the correct member name
        
        scheduler.set_config(config);
        
        std::cout << "4. Scheduler configuration:" << std::endl;
        std::cout << "   - Symbols: " << config.symbols.size() << " symbol(s): ";
        for (size_t i = 0; i < config.symbols.size(); ++i) {
            std::cout << config.symbols[i];
            if (i < config.symbols.size() - 1) std::cout << ", ";
        }
        std::cout << std::endl;
        std::cout << "   - Schedule: Daily at " << config.daily_hour << ":" 
                  << std::setfill('0') << std::setw(2) << config.daily_minute << " ET" << std::endl;
        std::cout << "   - Trading days: Sun-Thu" << std::endl;
        std::cout << "   - Initial load: " << config.initial_bars_daily << " bars" << std::endl;
        std::cout << "   - Recurring fetch: " << config.recurring_bars << " bar (latest only)" << std::endl;
        
        // Show menu
        while (true) {
            std::cout << "\n==============================================\n";
            std::cout << "NEXDAY SCHEDULER CONTROL MENU\n";
            std::cout << "==============================================\n";
            std::cout << "1. Start automated scheduler\n";
            std::cout << "2. Stop scheduler\n";
            std::cout << "3. Fetch all data now (manual)\n";
            std::cout << "4. Fetch daily data now\n";
            std::cout << "5. Check and recover missing data\n";
            std::cout << "6. Show status summary\n";
            std::cout << "7. Test comprehensive data fetch (all timeframes)\n";
            std::cout << "8. Show database table sizes\n";
            std::cout << "9. Test scheduler fetch operations\n";
            std::cout << "10. Exit\n";
            std::cout << "==============================================\n";
            
            if (scheduler.is_running()) {
                std::cout << "SCHEDULER STATUS: RUNNING" << std::endl;
            } else {
                std::cout << "SCHEDULER STATUS: STOPPED" << std::endl;
            }
            
            std::cout << "\nEnter choice (1-10): ";
            int choice;
            std::cin >> choice;
            
            switch (choice) {
                case 1: {
                    if (!scheduler.is_running()) {
                        if (scheduler.start_scheduler()) {
                            std::cout << "Scheduler started successfully!" << std::endl;
                            std::cout << "The scheduler will now run automatically." << std::endl;
                            std::cout << "Press any key to return to menu..." << std::endl;
                            std::cin.ignore();
                            std::cin.get();
                        } else {
                            std::cout << "Failed to start scheduler" << std::endl;
                        }
                    } else {
                        std::cout << "Scheduler is already running" << std::endl;
                    }
                    break;
                }
                
                case 2: {
                    if (scheduler.is_running()) {
                        scheduler.stop_scheduler();
                        std::cout << "Scheduler stopped" << std::endl;
                    } else {
                        std::cout << "Scheduler is not running" << std::endl;
                    }
                    break;
                }
                
                case 3: {
                    std::cout << "Fetching all data for all symbols..." << std::endl;
                    if (scheduler.fetch_all_data_now()) {
                        std::cout << "All data fetched successfully!" << std::endl;
                    } else {
                        std::cout << "Some fetches may have failed. Check logs for details." << std::endl;
                    }
                    break;
                }
                
                case 4: {
                    std::cout << "Fetching daily data for all symbols..." << std::endl;
                    if (scheduler.fetch_daily_data_now()) {
                        std::cout << "Daily data fetched successfully!" << std::endl;
                    } else {
                        std::cout << "Some daily fetches may have failed." << std::endl;
                    }
                    break;
                }
                
                case 5: {
                    std::cout << "Checking for missing data and recovering..." << std::endl;
                    if (scheduler.check_and_recover_today()) {
                        std::cout << "Recovery operation completed!" << std::endl;
                    } else {
                        std::cout << "Recovery operation had some issues." << std::endl;
                    }
                    break;
                }
                
                case 6: {
                    scheduler.print_status_summary();
                    scheduler.log_fetch_summary();
                    break;
                }
                
                case 7: {
                    std::cout << "Testing comprehensive data fetch (QGC# - All Timeframes)..." << std::endl;
                    std::cout << "========================================" << std::endl;
                    
                    bool overall_success = true;
                    int total_bars_saved = 0;
                    
                    // Test 1: Daily fetch
                    std::cout << "\n1. Testing Daily Data Fetch:" << std::endl;
                    auto daily_fetcher = std::make_unique<DailyDataFetcher>(connection_manager);
                    std::vector<HistoricalBar> daily_bars;
                    
                    if (daily_fetcher->fetch_historical_data("QGC#", config.bars_daily, daily_bars)) {
                        std::cout << "   Daily fetch successful: " << daily_bars.size() << " bars" << std::endl;
                        if (!daily_bars.empty()) {
                            // FIXED: Always use [0] for latest (newest first ordering)
                            const auto& bar = daily_bars[0];
                            std::cout << "   Latest daily bar: " << bar.date << " OHLC: " 
                                     << bar.open << "/" << bar.high << "/" 
                                     << bar.low << "/" << bar.close << std::endl;
                        }
                        
                        // Save to database
                        int saved = 0;
                        for (const auto& bar : daily_bars) {
                            if (db_manager->insert_historical_data("QGC#", bar.date, 
                                bar.open, bar.high, bar.low, bar.close, bar.volume)) {
                                saved++;
                            }
                        }
                        std::cout << "   Saved daily data: " << saved << "/" << daily_bars.size() << " bars" << std::endl;
                        total_bars_saved += saved;
                    } else {
                        std::cout << "   Daily fetch FAILED" << std::endl;
                        overall_success = false;
                    }
                    
                    // Test 2: 15-minute fetch
                    std::cout << "\n2. Testing 15-Minute Data Fetch:" << std::endl;
                    auto fifteen_min_fetcher = std::make_unique<FifteenMinDataFetcher>(connection_manager);
                    std::vector<HistoricalBar> fifteen_min_bars;
                    
                    if (fifteen_min_fetcher->fetch_historical_data("QGC#", config.bars_15min, fifteen_min_bars)) {
                        std::cout << "   15-min fetch successful: " << fifteen_min_bars.size() << " bars" << std::endl;
                        if (!fifteen_min_bars.empty()) {
                            // FIXED: Always use [0] for latest (newest first ordering)
                            const auto& bar = fifteen_min_bars[0];
                            std::cout << "   Latest 15-min bar: " << bar.date << " " << bar.time << " OHLC: " 
                                     << bar.open << "/" << bar.high << "/" 
                                     << bar.low << "/" << bar.close << std::endl;
                        }
                        
                        // Note: For now, using the same insert method - you'll need specific 15min methods later
                        int saved = fifteen_min_bars.size(); // Assume all saved for now
                        std::cout << "   15-min bars processed: " << saved << " bars" << std::endl;
                        total_bars_saved += saved;
                    } else {
                        std::cout << "   15-minute fetch FAILED" << std::endl;
                        overall_success = false;
                    }
                    
                    // Test 3: 30-minute fetch
                    std::cout << "\n3. Testing 30-Minute Data Fetch:" << std::endl;
                    auto thirty_min_fetcher = std::make_unique<ThirtyMinDataFetcher>(connection_manager);
                    std::vector<HistoricalBar> thirty_min_bars;
                    
                    if (thirty_min_fetcher->fetch_historical_data("QGC#", config.bars_30min, thirty_min_bars)) {
                        std::cout << "   30-min fetch successful: " << thirty_min_bars.size() << " bars" << std::endl;
                        if (!thirty_min_bars.empty()) {
                            // FIXED: Always use [0] for latest (newest first ordering)
                            const auto& bar = thirty_min_bars[0];
                            std::cout << "   Latest 30-min bar: " << bar.date << " " << bar.time << " OHLC: " 
                                     << bar.open << "/" << bar.high << "/" 
                                     << bar.low << "/" << bar.close << std::endl;
                        }
                        
                        int saved = thirty_min_bars.size(); // Assume all processed
                        std::cout << "   30-min bars processed: " << saved << " bars" << std::endl;
                        total_bars_saved += saved;
                    } else {
                        std::cout << "   30-minute fetch FAILED" << std::endl;
                        overall_success = false;
                    }
                    
                    // Test 4: 1-hour fetch
                    std::cout << "\n4. Testing 1-Hour Data Fetch:" << std::endl;
                    auto one_hour_fetcher = std::make_unique<OneHourDataFetcher>(connection_manager);
                    std::vector<HistoricalBar> one_hour_bars;
                    
                    if (one_hour_fetcher->fetch_historical_data("QGC#", config.bars_1hour, one_hour_bars)) {
                        std::cout << "   1-hour fetch successful: " << one_hour_bars.size() << " bars" << std::endl;
                        if (!one_hour_bars.empty()) {
                            // FIXED: Always use [0] for latest (newest first ordering)
                            const auto& bar = one_hour_bars[0];
                            std::cout << "   Latest 1-hour bar: " << bar.date << " " << bar.time << " OHLC: " 
                                     << bar.open << "/" << bar.high << "/" 
                                     << bar.low << "/" << bar.close << std::endl;
                        }
                        
                        int saved = one_hour_bars.size(); // Assume all processed
                        std::cout << "   1-hour bars processed: " << saved << " bars" << std::endl;
                        total_bars_saved += saved;
                    } else {
                        std::cout << "   1-hour fetch FAILED" << std::endl;
                        overall_success = false;
                    }
                    
                    // Test 5: 2-hour fetch
                    std::cout << "\n5. Testing 2-Hour Data Fetch:" << std::endl;
                    auto two_hour_fetcher = std::make_unique<TwoHourDataFetcher>(connection_manager);
                    std::vector<HistoricalBar> two_hour_bars;
                    
                    if (two_hour_fetcher->fetch_historical_data("QGC#", config.bars_2hours, two_hour_bars)) {
                        std::cout << "   2-hour fetch successful: " << two_hour_bars.size() << " bars" << std::endl;
                        if (!two_hour_bars.empty()) {
                            // FIXED: Always use [0] for latest (newest first ordering)
                            const auto& bar = two_hour_bars[0];
                            std::cout << "   Latest 2-hour bar: " << bar.date << " " << bar.time << " OHLC: " 
                                     << bar.open << "/" << bar.high << "/" 
                                     << bar.low << "/" << bar.close << std::endl;
                        }
                        
                        int saved = two_hour_bars.size(); // Assume all processed
                        std::cout << "   2-hour bars processed: " << saved << " bars" << std::endl;
                        total_bars_saved += saved;
                    } else {
                        std::cout << "   2-hour fetch FAILED" << std::endl;
                        overall_success = false;
                    }
                    
                    // Summary
                    std::cout << "\n========================================" << std::endl;
                    std::cout << "COMPREHENSIVE TEST SUMMARY:" << std::endl;
                    std::cout << "   Overall Status: " << (overall_success ? "SUCCESS" : "PARTIAL/FAILED") << std::endl;
                    std::cout << "   Total Bars Processed: " << total_bars_saved << std::endl;
                    std::cout << "   All Timeframes Tested: Daily, 15min, 30min, 1hour, 2hour" << std::endl;
                    std::cout << "========================================" << std::endl;
                    
                    std::cout << "\nPress Enter to continue...";
                    std::cin.ignore();
                    std::cin.get();
                    break;
                }
                
                case 8: {
                    std::cout << "Database table sizes:" << std::endl;
                    db_manager->print_table_sizes();
                    break;
                }
                
                case 9: {
                    std::cout << "Testing scheduler fetch operations..." << std::endl;
                    std::cout << "========================================" << std::endl;
                    
                    // Test the scheduler's fetch methods directly
                    std::cout << "\n1. Testing Scheduler Daily Fetch:" << std::endl;
                    if (scheduler.fetch_daily_data_now("QGC#")) {
                        std::cout << "   Scheduler daily fetch: SUCCESS" << std::endl;
                    } else {
                        std::cout << "   Scheduler daily fetch: FAILED" << std::endl;
                    }
                    
                    std::cout << "\n2. Testing Scheduler Intraday Fetches:" << std::endl;
                    std::vector<std::string> timeframes = {"15min", "30min", "1hour", "2hours"};
                    
                    for (const auto& tf : timeframes) {
                        if (scheduler.fetch_intraday_data_now(tf, "QGC#")) {
                            std::cout << "   Scheduler " << tf << " fetch: SUCCESS" << std::endl;
                        } else {
                            std::cout << "   Scheduler " << tf << " fetch: FAILED" << std::endl;
                        }
                    }
                    
                    std::cout << "\n3. Testing Full Data Fetch via Scheduler:" << std::endl;
                    if (scheduler.fetch_all_data_now("QGC#")) {
                        std::cout << "   Scheduler comprehensive fetch: SUCCESS" << std::endl;
                    } else {
                        std::cout << "   Scheduler comprehensive fetch: FAILED" << std::endl;
                    }
                    
                    std::cout << "\n========================================" << std::endl;
                    std::cout << "SCHEDULER TEST COMPLETED" << std::endl;
                    std::cout << "Check logs directory for detailed fetch information." << std::endl;
                    std::cout << "========================================" << std::endl;
                    
                    std::cout << "\nPress Enter to continue...";
                    std::cin.ignore();
                    std::cin.get();
                    break;
                }
                
                case 10: {
                    std::cout << "Shutting down..." << std::endl;
                    if (scheduler.is_running()) {
                        std::cout << "Stopping scheduler..." << std::endl;
                        scheduler.stop_scheduler();
                    }
                    std::cout << "Goodbye!" << std::endl;
                    return 0;
                }
                
                default: {
                    std::cout << "Invalid choice. Please enter 1-10." << std::endl;
                    break;
                }
            }
            
            // Brief pause before showing menu again
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Exception occurred: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}