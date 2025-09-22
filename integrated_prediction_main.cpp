#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <iomanip>

#include "IntegratedMarketPredictionEngine.h"
#include "database_simple.h"
#include "IQFeedConnectionManager.h"

void print_header() {
    std::cout << "\n";
    std::cout << "================================================================\n";
    std::cout << "   NEXDAY MARKETS - INTEGRATED PREDICTION ENGINE\n";
    std::cout << "   Model 1 Standard: EMA-Based Price Predictions\n";
    std::cout << "   Connected to Real Database & IQFeed Data Pipeline\n";
    std::cout << "================================================================\n";
}

void print_menu() {
    std::cout << "\n================================================================\n";
    std::cout << "PREDICTION ENGINE MENU\n";
    std::cout << "================================================================\n";
    std::cout << "1. Generate predictions for single symbol\n";
    std::cout << "2. Generate daily predictions only\n";
    std::cout << "3. Generate intraday predictions only\n";
    std::cout << "4. Generate predictions for all active symbols\n";
    std::cout << "5. Test system integration (QGC# sample)\n";
    std::cout << "6. View system status\n";
    std::cout << "7. Test database historical data retrieval\n";
    std::cout << "8. Validate EMA calculation details\n";
    std::cout << "9. View recent predictions from database\n";
    std::cout << "10. Exit\n";
    std::cout << "================================================================\n";
}

void test_system_integration(IntegratedMarketPredictionEngine& engine) {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "SYSTEM INTEGRATION TEST" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    // Test with QGC# (Gold futures)
    std::string test_symbol = "QGC#";
    std::cout << "Testing with symbol: " << test_symbol << std::endl;
    std::cout << "This will use REAL historical data from your database" << std::endl;
    std::cout << "and generate REAL predictions using Model 1 Standard EMA" << std::endl;
    
    std::cout << "\nPress Enter to continue...";
    std::cin.ignore();
    std::cin.get();
    
    engine.print_system_status();
    
    if (engine.is_ready()) {
        std::cout << "\n🔥 GENERATING REAL PREDICTIONS..." << std::endl;
        
        if (engine.generate_predictions_for_symbol(test_symbol)) {
            std::cout << "\n✅ SUCCESS! Real predictions generated using:" << std::endl;
            std::cout << "   • Historical data from historical_fetch_daily table" << std::endl;
            std::cout << "   • Model 1 Standard EMA algorithm (base_alpha=0.5)" << std::endl;
            std::cout << "   • Predictions saved to predictions_all_symbols table" << std::endl;
            std::cout << "   • Both daily OHLC and intraday High/Low predictions" << std::endl;
        } else {
            std::cout << "\n❌ FAILED! Check logs for details." << std::endl;
            std::cout << "Common issues:" << std::endl;
            std::cout << "   • No historical data in database for " << test_symbol << std::endl;
            std::cout << "   • Database connection problems" << std::endl;
            std::cout << "   • Insufficient data (need 15+ bars minimum)" << std::endl;
        }
    } else {
        std::cout << "\n❌ SYSTEM NOT READY!" << std::endl;
        std::cout << "Please check database and IQFeed connections" << std::endl;
    }
    
    std::cout << "\nPress Enter to continue...";
    std::cin.get();
}

void view_recent_predictions(std::shared_ptr<SimpleDatabaseManager> db_manager) {
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "RECENT PREDICTIONS FROM DATABASE" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    
    // Since execute_query_with_result is private, we'll show instructions instead
    std::cout << "Database query functionality requires access fix." << std::endl;
    std::cout << "\nTo enable this feature:" << std::endl;
    std::cout << "1. Edit Database/database_simple.h" << std::endl;
    std::cout << "2. Move execute_query_with_result from private to public section" << std::endl;
    std::cout << "3. Rebuild the project" << std::endl;
    
    // For now, show we can at least get the symbol list
    try {
        std::vector<std::string> symbols = db_manager->get_symbol_list(true);
        std::cout << "\nAvailable symbols in database: " << symbols.size() << std::endl;
        for (const auto& symbol : symbols) {
            std::cout << "  - " << symbol << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Error getting symbol list: " << e.what() << std::endl;
    }
    
    std::cout << "\nPress Enter to continue...";
    std::cin.ignore();
    std::cin.get();
}

void test_database_retrieval(IntegratedMarketPredictionEngine& engine, 
                           std::shared_ptr<SimpleDatabaseManager> db_manager) {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "DATABASE HISTORICAL DATA RETRIEVAL TEST" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    std::cout << "Enter symbol to test (e.g., QGC#, AAPL): ";
    std::string symbol;
    std::getline(std::cin, symbol);
    
    if (symbol.empty()) {
        symbol = "QGC#";
        std::cout << "Using default symbol: " << symbol << std::endl;
    }
    
    std::cout << "\nTesting database connection and symbol availability..." << std::endl;
    
    // Test basic database functionality we can access
    try {
        // Get symbol list to test database connectivity
        std::vector<std::string> symbols = db_manager->get_symbol_list(true);
        std::cout << "Database connection: OK" << std::endl;
        std::cout << "Active symbols in database: " << symbols.size() << std::endl;
        
        bool symbol_found = false;
        for (const auto& db_symbol : symbols) {
            if (db_symbol == symbol) {
                symbol_found = true;
                break;
            }
        }
        
        if (symbol_found) {
            std::cout << "Symbol " << symbol << " found in database: YES" << std::endl;
            std::cout << "\nTo test full data retrieval functionality:" << std::endl;
            std::cout << "1. Fix database access (see instructions below)" << std::endl;
            std::cout << "2. Then use the prediction generation features" << std::endl;
        } else {
            std::cout << "Symbol " << symbol << " found in database: NO" << std::endl;
            std::cout << "\nTo fix this:" << std::endl;
            std::cout << "1. Run your main data collection program" << std::endl;
            std::cout << "2. Use option 7 (Test comprehensive data fetch)" << std::endl;
            std::cout << "3. This will populate historical data tables" << std::endl;
        }
        
        // Show current available symbols
        if (!symbols.empty()) {
            std::cout << "\nAvailable symbols:" << std::endl;
            for (const auto& sym : symbols) {
                std::cout << "  - " << sym << std::endl;
            }
        }
        
    } catch (const std::exception& e) {
        std::cout << "Database test error: " << e.what() << std::endl;
    }
    
    std::cout << "\nPress Enter to continue...";
    std::cin.get();
}

int main() {
    print_header();
    
    std::cout << "Initializing Integrated Prediction Engine...\n" << std::endl;
    
    try {
        // Step 1: Initialize Database Connection
        std::cout << "1. Connecting to PostgreSQL database..." << std::endl;
        DatabaseConfig db_config;
        db_config.host = "localhost";
        db_config.port = 5432;
        db_config.database = "nexday_trading";
        db_config.username = "nexday_user";  
        db_config.password = "nexday_secure_password_2025";
        
        auto db_manager = std::make_shared<SimpleDatabaseManager>(db_config);
        
        if (!db_manager->test_connection()) {
            std::cout << "❌ Database connection failed!" << std::endl;
            std::cout << "\nTroubleshooting:" << std::endl;
            std::cout << "• Ensure PostgreSQL is running" << std::endl;
            std::cout << "• Check database exists: nexday_trading" << std::endl;
            std::cout << "• Verify credentials are correct" << std::endl;
            std::cout << "• Run: cmake --build . --target init_database" << std::endl;
            return 1;
        }
        std::cout << "✅ Database connected successfully" << std::endl;
        
        // Step 2: Initialize IQFeed Connection  
        std::cout << "2. Connecting to IQFeed..." << std::endl;
        auto iqfeed_manager = std::make_shared<IQFeedConnectionManager>();
        
        if (!iqfeed_manager->initialize_connection()) {
            std::cout << "⚠️  IQFeed connection failed (will use database-only mode)" << std::endl;
            std::cout << "   • For real-time updates, ensure IQConnect.exe is running" << std::endl;
            std::cout << "   • Predictions will use existing database data" << std::endl;
        } else {
            std::cout << "✅ IQFeed connected successfully" << std::endl;
        }
        
        // Step 3: Initialize Integrated Prediction Engine
        std::cout << "3. Initializing prediction engine..." << std::endl;
        IntegratedMarketPredictionEngine engine(db_manager, iqfeed_manager);
        std::cout << "✅ Prediction engine ready" << std::endl;
        
        // Check system readiness
        engine.print_system_status();
        
        if (!engine.is_ready()) {
            std::cout << "\n⚠️  System partially ready - some features may be limited" << std::endl;
            std::cout << "Database predictions will work if historical data is available" << std::endl;
        }
        
        // Main menu loop
        int choice;
        std::string symbol;
        
        while (true) {
            print_menu();
            std::cout << "Choose option: ";
            
            if (!(std::cin >> choice)) {
                std::cin.clear();
                std::cin.ignore(10000, '\n');
                std::cout << "Invalid input. Please enter a number.\n";
                continue;
            }
            
            std::cin.ignore(); // Clear newline
            
            switch (choice) {
                case 1: {
                    std::cout << "Enter symbol (e.g., QGC#, AAPL): ";
                    std::getline(std::cin, symbol);
                    
                    if (!symbol.empty()) {
                        std::cout << "\nGenerating comprehensive predictions for " << symbol << "...\n";
                        
                        if (engine.generate_predictions_for_symbol(symbol)) {
                            std::cout << "\n✅ All predictions generated successfully!" << std::endl;
                        } else {
                            std::cout << "\n❌ Some predictions failed. Check logs for details." << std::endl;
                        }
                    }
                    break;
                }
                
                case 2: {
                    std::cout << "Enter symbol (e.g., QGC#): ";
                    std::getline(std::cin, symbol);
                    
                    if (!symbol.empty()) {
                        std::cout << "\nGenerating daily OHLC predictions for " << symbol << "...\n";
                        
                        if (engine.generate_daily_prediction(symbol)) {
                            std::cout << "\n✅ Daily predictions generated!" << std::endl;
                        } else {
                            std::cout << "\n❌ Daily prediction failed." << std::endl;
                        }
                    }
                    break;
                }
                
                case 3: {
                    std::cout << "Enter symbol (e.g., QGC#): ";
                    std::getline(std::cin, symbol);
                    
                    if (!symbol.empty()) {
                        std::cout << "Select timeframe:" << std::endl;
                        std::cout << "1. 15min  2. 30min  3. 1hour  4. 2hours" << std::endl;
                        std::cout << "Choice: ";
                        
                        int tf_choice;
                        std::cin >> tf_choice;
                        std::cin.ignore();
                        
                        std::string timeframe;
                        switch (tf_choice) {
                            case 1: timeframe = "15min"; break;
                            case 2: timeframe = "30min"; break;
                            case 3: timeframe = "1hour"; break;
                            case 4: timeframe = "2hours"; break;
                            default: timeframe = "15min";
                        }
                        
                        std::cout << "\nGenerating " << timeframe << " predictions for " << symbol << "...\n";
                        
                        if (engine.generate_intraday_prediction(symbol, timeframe)) {
                            std::cout << "\n✅ " << timeframe << " predictions generated!" << std::endl;
                        } else {
                            std::cout << "\n❌ " << timeframe << " prediction failed." << std::endl;
                        }
                    }
                    break;
                }
                
                case 4: {
                    std::cout << "\nGenerating predictions for ALL active symbols..." << std::endl;
                    std::cout << "⚠️  This may take several minutes..." << std::endl;
                    
                    std::vector<std::string> symbols = db_manager->get_symbol_list(true);
                    std::cout << "Found " << symbols.size() << " active symbols" << std::endl;
                    
                    if (!symbols.empty()) {
                        int success_count = 0;
                        for (const auto& sym : symbols) {
                            std::cout << "Processing " << sym << "..." << std::endl;
                            if (engine.generate_predictions_for_symbol(sym)) {
                                success_count++;
                            }
                        }
                        
                        std::cout << "\n✅ Completed: " << success_count << "/" << symbols.size() 
                                  << " symbols processed successfully" << std::endl;
                    } else {
                        std::cout << "No active symbols found in database" << std::endl;
                    }
                    break;
                }
                
                case 5: {
                    test_system_integration(engine);
                    break;
                }
                
                case 6: {
                    engine.print_system_status();
                    std::cout << "\nPress Enter to continue...";
                    std::cin.get();
                    break;
                }
                
                case 7: {
                    test_database_retrieval(engine, db_manager);
                    break;
                }
                
                case 8: {
                    std::cout << "Enter symbol for EMA validation (e.g., QGC#): ";
                    std::getline(std::cin, symbol);
                    
                    if (!symbol.empty()) {
                        engine.print_ema_calculation_details(symbol);
                    }
                    
                    std::cout << "\nPress Enter to continue...";
                    std::cin.get();
                    break;
                }
                
                case 9: {
                    view_recent_predictions(db_manager);
                    break;
                }
                
                case 10: {
                    std::cout << "\nShutting down Integrated Prediction Engine..." << std::endl;
                    std::cout << "Thank you for using Nexday Markets Prediction System!" << std::endl;
                    return 0;
                }
                
                default: {
                    std::cout << "Invalid option. Please choose 1-10." << std::endl;
                    break;
                }
            }
            
            std::cout << "\nPress Enter to continue...";
            std::cin.get();
        }
        
    } catch (const std::exception& e) {
        std::cout << "❌ Fatal error: " << e.what() << std::endl;
        std::cout << "\nPress Enter to exit...";
        std::cin.get();
        return 1;
    }
    
    return 0;
}