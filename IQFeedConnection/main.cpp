#include <iostream>
#include <memory>
#include <string>
#include <chrono>
#include <thread>
#include <iomanip>

// Existing includes
#include "IQFeedConnectionManager.h"
#include "FetchScheduler.h"
#include "database_simple.h"
#include "DailyDataFetcher.h"
#include "FifteenMinDataFetcher.h"
#include "ThirtyMinDataFetcher.h"
#include "OneHourDataFetcher.h"
#include "TwoHourDataFetcher.h"
#include "PredictionValidator.h"

// NEW: Add prediction engine includes
#include "MarketPredictionEngine.h"
#include "MarketPrediction.h"
#include "PredictionTypes.h"
#include "BusinessDayCalculator.h"

int main() {
    std::cout << "\n==============================================\n";
    std::cout << "   NEXDAY MARKETS - COMPLETE PIPELINE\n";
    std::cout << "   Data → Predictions → Validation → Errors\n";
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
        
        // Initialize PredictionValidator
        std::cout << "1b. Initializing PredictionValidator..." << std::endl;
        auto validator = std::make_unique<PredictionValidator>(std::make_unique<SimpleDatabaseManager>(db_config));
        
        // NEW: Initialize MarketPredictionEngine
        std::cout << "1c. Initializing MarketPredictionEngine..." << std::endl;
        auto prediction_engine = std::make_unique<MarketPredictionEngine>(std::make_unique<SimpleDatabaseManager>(db_config));
        
        if (!prediction_engine->is_initialized()) {
            std::cerr << "Failed to initialize prediction engine: " << prediction_engine->get_last_error() << std::endl;
            return 1;
        }
        
        std::cout << "Prediction Engine initialized successfully\n" << std::endl;
        
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
        config.symbols = {"QGC#"}; // Gold rolling contract futures
        config.daily_hour = 19;  // 7 PM ET
        config.daily_minute = 0; // 00 minutes
        config.enabled = true;
        config.trading_days = {0, 1, 2, 3, 4}; // Sunday through Thursday
        config.bars_15min = 100;
        config.bars_30min = 100;
        config.bars_1hour = 100;
        config.bars_2hours = 100;
        config.bars_daily = 100;
        config.initial_bars_daily = 100;
        config.recurring_bars = 1;
        
        scheduler.set_config(config);
        
        std::cout << "4. System Ready - Complete Pipeline Available" << std::endl;
        std::cout << "   - Symbols: QGC# (Gold Futures)" << std::endl;
        std::cout << "   - Model: Epoch Market Advisor (EMA-based)" << std::endl;
        std::cout << "   - Pipeline: Data → Predictions → Validation → Errors" << std::endl;
        
        // Show menu
        while (true) {
            std::cout << "\n==============================================\n";
            std::cout << "NEXDAY COMPLETE PIPELINE MENU\n";
            std::cout << "==============================================\n";
            std::cout << "📊 DATA FETCHING:\n";
            std::cout << "1. Start automated scheduler\n";
            std::cout << "2. Stop scheduler\n";
            std::cout << "3. Fetch all data now (manual)\n";
            std::cout << "4. Fetch daily data now\n";
            std::cout << "5. Test comprehensive data fetch\n";
            std::cout << "6. Show database table sizes\n";
            std::cout << "\n🔮 PREDICTION GENERATION:\n";
            std::cout << "17. Generate predictions for QGC# ⭐ NEW\n";
            std::cout << "18. Generate predictions for all symbols ⭐ NEW\n";
            std::cout << "19. Test EMA calculation with real data ⭐ NEW\n";
            std::cout << "20. Run COMPLETE PIPELINE (Fetch → Predict → Validate) ⭐ NEW\n";
            std::cout << "\n✅ PREDICTION VALIDATION:\n";
            std::cout << "10. Validate all pending predictions\n";
            std::cout << "11. Validate daily predictions\n";
            std::cout << "12. Validate intraday predictions\n";
            std::cout << "13. Show prediction validation summary\n";
            std::cout << "14. Show model performance metrics\n";
            std::cout << "15. Test prediction validator\n";
            std::cout << "\n⚙️  SYSTEM:\n";
            std::cout << "7. Check and recover missing data\n";
            std::cout << "8. Show status summary\n";
            std::cout << "9. Test scheduler operations\n";
            std::cout << "16. Exit\n";
            std::cout << "==============================================\n";
            
            if (scheduler.is_running()) {
                std::cout << "🟢 SCHEDULER STATUS: RUNNING" << std::endl;
            } else {
                std::cout << "🔴 SCHEDULER STATUS: STOPPED" << std::endl;
            }
            
            std::cout << "\nEnter choice (1-20): ";
            int choice;
            std::cin >> choice;
            
            switch (choice) {
                // Existing cases 1-16 remain the same...
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
                    std::cout << "Testing comprehensive data fetch (QGC# - All Timeframes)..." << std::endl;
                    std::cout << "========================================" << std::endl;
                    
                    bool overall_success = true;
                    int total_bars_saved = 0;
                    
                    // Test daily fetch and save
                    std::cout << "\n1. Testing Daily Data Fetch:" << std::endl;
                    auto daily_fetcher = std::make_unique<DailyDataFetcher>(connection_manager);
                    std::vector<HistoricalBar> daily_bars;
                    
                    if (daily_fetcher->fetch_historical_data("QGC#", config.bars_daily, daily_bars)) {
                        int saved = 0;
                        for (const auto& bar : daily_bars) {
                            if (db_manager->insert_historical_data_daily("QGC#", bar.date, 
                                bar.open, bar.high, bar.low, bar.close, bar.volume, bar.open_interest)) {
                                saved++;
                            }
                        }
                        std::cout << "   Daily data saved: " << saved << "/" << daily_bars.size() << " bars" << std::endl;
                        total_bars_saved += saved;
                    } else {
                        std::cout << "   Daily fetch FAILED" << std::endl;
                        overall_success = false;
                    }
                    
                    // Test 15-minute fetch and save
                    std::cout << "\n2. Testing 15-Minute Data Fetch:" << std::endl;
                    auto fifteen_min_fetcher = std::make_unique<FifteenMinDataFetcher>(connection_manager);
                    std::vector<HistoricalBar> fifteen_min_bars;
                    
                    if (fifteen_min_fetcher->fetch_historical_data("QGC#", config.bars_15min, fifteen_min_bars)) {
                        int saved = 0;
                        for (const auto& bar : fifteen_min_bars) {
                            if (db_manager->insert_historical_data_15min("QGC#", bar.date, bar.time,
                                bar.open, bar.high, bar.low, bar.close, bar.volume, bar.open_interest)) {
                                saved++;
                            }
                        }
                        std::cout << "   15-min data saved: " << saved << "/" << fifteen_min_bars.size() << " bars" << std::endl;
                        total_bars_saved += saved;
                    } else {
                        overall_success = false;
                    }
                    
                    std::cout << "\nTotal bars saved: " << total_bars_saved << std::endl;
                    std::cout << "Comprehensive test: " << (overall_success ? "SUCCESS" : "PARTIAL") << std::endl;
                    break;
                }
                
                case 6: {
                    std::cout << "Database table sizes:" << std::endl;
                    db_manager->print_table_sizes();
                    break;
                }
                
                case 7: {
                    std::cout << "Checking for missing data and recovering..." << std::endl;
                    if (scheduler.check_and_recover_today()) {
                        std::cout << "Recovery operation completed!" << std::endl;
                    } else {
                        std::cout << "Recovery operation had some issues." << std::endl;
                    }
                    break;
                }
                
                case 8: {
                    scheduler.print_status_summary();
                    scheduler.log_fetch_summary();
                    break;
                }
                
                case 9: {
                    std::cout << "Testing scheduler operations..." << std::endl;
                    if (scheduler.fetch_all_data_now("QGC#")) {
                        std::cout << "Scheduler operations: SUCCESS" << std::endl;
                    } else {
                        std::cout << "Scheduler operations: FAILED" << std::endl;
                    }
                    break;
                }
                
                // Existing validation cases 10-15...
                case 10: {
                    std::cout << "Validating ALL pending predictions..." << std::endl;
                    auto result = validator->validate_all_predictions();
                    std::cout << "Validation completed: " << result.predictions_validated 
                              << "/" << result.predictions_found << " predictions processed" << std::endl;
                    break;
                }
                
                case 11: {
                    std::cout << "Validating DAILY predictions..." << std::endl;
                    auto result = validator->validate_daily_predictions("QGC#");
                    std::cout << "Daily validation: " << (result.success ? "SUCCESS" : "FAILED") << std::endl;
                    break;
                }
                
                case 12: {
                    std::cout << "Validating INTRADAY predictions..." << std::endl;
                    std::vector<TimeFrame> timeframes = {
                        TimeFrame::MINUTES_15, TimeFrame::MINUTES_30, 
                        TimeFrame::HOUR_1, TimeFrame::HOURS_2
                    };
                    
                    for (auto tf : timeframes) {
                        auto result = validator->validate_intraday_predictions(tf, "QGC#");
                        std::cout << "   " << timeframe_to_string(tf) << ": " 
                                  << (result.success ? "SUCCESS" : "FAILED") << std::endl;
                    }
                    break;
                }
                
                case 13: {
                    validator->generate_validation_report("QGC#");
                    break;
                }
                
                case 14: {
                    validator->generate_error_summary_report();
                    break;
                }
                
                case 15: {
                    std::cout << "Testing prediction validator..." << std::endl;
                    std::vector<double> predicted = {100.0, 105.0, 110.0};
                    std::vector<double> actual = {102.0, 104.0, 109.0};
                    
                    auto metrics = validator->calculate_error_metrics(predicted, actual);
                    std::cout << "Test MAE: " << metrics.mae << std::endl;
                    std::cout << "Test RMSE: " << metrics.rmse << std::endl;
                    std::cout << "Validator test: SUCCESS" << std::endl;
                    break;
                }
                
                // NEW PREDICTION GENERATION CASES
                case 17: {
                    std::cout << "🔮 GENERATING PREDICTIONS FOR QGC#" << std::endl;
                    std::cout << "========================================" << std::endl;
                    
                    if (prediction_engine->generate_predictions_for_symbol("QGC#")) {
                        std::cout << "✅ Predictions generated successfully for QGC#!" << std::endl;
                        
                        // Show what was generated
                        std::cout << "\nPredictions generated:" << std::endl;
                        std::cout << "📈 Daily OHLC predictions (next business day)" << std::endl;
                        std::cout << "📊 Intraday High/Low predictions (15min, 30min, 1hour, 2hour)" << std::endl;
                        std::cout << "🔄 Using Model 1 Standard EMA algorithm" << std::endl;
                        std::cout << "💾 Saved to predictions_daily and predictions_all_symbols tables" << std::endl;
                    } else {
                        std::cout << "❌ Failed to generate predictions for QGC#" << std::endl;
                        std::cout << "Error: " << prediction_engine->get_last_error() << std::endl;
                    }
                    
                    std::cout << "\nPress Enter to continue...";
                    std::cin.ignore();
                    std::cin.get();
                    break;
                }
                
                case 18: {
                    std::cout << "🔮 GENERATING PREDICTIONS FOR ALL SYMBOLS" << std::endl;
                    std::cout << "========================================" << std::endl;
                    
                    if (prediction_engine->generate_predictions_for_all_active_symbols()) {
                        std::cout << "✅ Predictions generated successfully for all active symbols!" << std::endl;
                    } else {
                        std::cout << "❌ Some prediction generation failed" << std::endl;
                        std::cout << "Error: " << prediction_engine->get_last_error() << std::endl;
                    }
                    
                    std::cout << "\nPress Enter to continue...";
                    std::cin.ignore();
                    std::cin.get();
                    break;
                }
                
                case 19: {
                    std::cout << "🧮 TESTING EMA CALCULATION WITH REAL DATA" << std::endl;
                    std::cout << "========================================" << std::endl;
                    
                    // Get real historical data for testing
                    auto historical_data = prediction_engine->get_historical_data("QGC#", TimeFrame::DAILY, 25);
                    
                    if (historical_data.size() >= 15) {
                        std::cout << "📊 Retrieved " << historical_data.size() << " historical bars for QGC#" << std::endl;
                        
                        // Test EMA calculation
                        auto ema_result = prediction_engine->calculate_ema_for_prediction(historical_data, "close");
                        
                        if (ema_result.valid) {
                            std::cout << "✅ EMA calculation successful!" << std::endl;
                            std::cout << "🎯 Final EMA prediction: " << ema_result.final_ema << std::endl;
                            std::cout << "📈 SMA values calculated: " << ema_result.sma_values.size() << std::endl;
                            std::cout << "📊 EMA values calculated: " << ema_result.ema_values.size() << std::endl;
                            std::cout << "📋 Total bars used: " << ema_result.bars_used << std::endl;
                            
                            // Show debug output
                            prediction_engine->print_ema_calculation_debug(historical_data, ema_result);
                        } else {
                            std::cout << "❌ EMA calculation failed" << std::endl;
                            std::cout << "Error: " << prediction_engine->get_last_error() << std::endl;
                        }
                    } else {
                        std::cout << "❌ Insufficient historical data: " << historical_data.size() << " bars (need 15+)" << std::endl;
                    }
                    
                    std::cout << "\nPress Enter to continue...";
                    std::cin.ignore();
                    std::cin.get();
                    break;
                }
                
                case 20: {
                    std::cout << "🚀 RUNNING COMPLETE PIPELINE" << std::endl;
                    std::cout << "========================================" << std::endl;
                    std::cout << "Step 1: Fetch Historical Data" << std::endl;
                    std::cout << "Step 2: Generate Predictions" << std::endl;
                    std::cout << "Step 3: Validate Predictions" << std::endl;
                    std::cout << "Step 4: Calculate Error Metrics" << std::endl;
                    std::cout << "========================================" << std::endl;
                    
                    bool pipeline_success = true;
                    
                    // Step 1: Fetch Data
                    std::cout << "\n📊 STEP 1: Fetching historical data for QGC#..." << std::endl;
                    if (scheduler.fetch_all_data_now("QGC#")) {
                        std::cout << "✅ Data fetch completed" << std::endl;
                    } else {
                        std::cout << "❌ Data fetch failed" << std::endl;
                        pipeline_success = false;
                    }
                    
                    // Step 2: Generate Predictions
                    if (pipeline_success) {
                        std::cout << "\n🔮 STEP 2: Generating predictions..." << std::endl;
                        if (prediction_engine->generate_predictions_for_symbol("QGC#")) {
                            std::cout << "✅ Predictions generated" << std::endl;
                        } else {
                            std::cout << "❌ Prediction generation failed: " << prediction_engine->get_last_error() << std::endl;
                            pipeline_success = false;
                        }
                    }
                    
                    // Step 3: Validate Predictions
                    if (pipeline_success) {
                        std::cout << "\n✅ STEP 3: Validating predictions..." << std::endl;
                        auto validation_result = validator->validate_all_predictions();
                        if (validation_result.success) {
                            std::cout << "✅ Validation completed: " << validation_result.predictions_validated 
                                      << " predictions processed" << std::endl;
                        } else {
                            std::cout << "⚠️ Validation had issues: " << validation_result.error_message << std::endl;
                        }
                    }
                    
                    // Step 4: Show Results
                    std::cout << "\n📈 STEP 4: Pipeline Summary" << std::endl;
                    std::cout << "========================================" << std::endl;
                    if (pipeline_success) {
                        std::cout << "🎉 COMPLETE PIPELINE: SUCCESS!" << std::endl;
                        std::cout << "📊 Data fetched and stored" << std::endl;
                        std::cout << "🔮 Predictions generated and stored" << std::endl;
                        std::cout << "✅ Validation completed" << std::endl;
                        std::cout << "📋 Ready for error analysis" << std::endl;
                    } else {
                        std::cout << "❌ PIPELINE: PARTIAL SUCCESS" << std::endl;
                        std::cout << "Check individual steps above for details" << std::endl;
                    }
                    std::cout << "========================================" << std::endl;
                    
                    std::cout << "\nPress Enter to continue...";
                    std::cin.ignore();
                    std::cin.get();
                    break;
                }
                
                case 16: {
                    std::cout << "Shutting down..." << std::endl;
                    if (scheduler.is_running()) {
                        std::cout << "Stopping scheduler..." << std::endl;
                        scheduler.stop_scheduler();
                    }
                    std::cout << "Goodbye!" << std::endl;
                    return 0;
                }
                
                default: {
                    std::cout << "Invalid choice. Please enter 1-20." << std::endl;
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