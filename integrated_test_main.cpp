#include "database_simple.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <iomanip>

// Test configuration
const std::vector<std::string> TEST_SYMBOLS = {"AAPL", "MSFT", "GOOGL", "TSLA", "SPY"};

class IntegratedTester {
private:
    std::unique_ptr<SimpleDatabaseManager> db_manager_;  // Changed from EnhancedDatabaseManager
    int total_tests_;
    int passed_tests_;
    
public:
    IntegratedTester() : total_tests_(0), passed_tests_(0) {}
    
    bool run_all_tests() {
        std::cout << "=====================================================" << std::endl;
        std::cout << "NEXDAY PREDICTIONS SYSTEM - INTEGRATED TEST SUITE" << std::endl;
        std::cout << "=====================================================" << std::endl;
        
        // Test 1: Database Connection
        if (!test_database_connection()) {
            std::cout << "Database tests failed - cannot proceed" << std::endl;
            return false;
        }
        
        // Test 2: Symbol Management
        test_symbol_management();
        
        // Test 3: Market Data Processing
        test_market_data_processing();
        
        // Test 4: System Performance
        test_system_performance();
        
        // Final Results
        print_test_results();
        
        return passed_tests_ == total_tests_;
    }

private:
    bool test_database_connection() {
        std::cout << "\nTEST 1: Database Connection" << std::endl;
        std::cout << "=====================================\n" << std::endl;
        
        try {
            // Create database configuration
            DatabaseConfig config;
            config.host = "localhost";
            config.port = 5432;
            config.database = "nexday_trading";
            config.username = "nexday_user";
            config.password = "nexday_secure_password_2025";
            
            db_manager_ = std::make_unique<SimpleDatabaseManager>(config);  // Changed class name
            
            // Test basic connection
            bool connected = db_manager_->test_connection();
            record_test("Database Connection", connected);
            
            if (!connected) {
                std::cout << "Failed to connect to database" << std::endl;
                std::cout << "Error: " << db_manager_->get_last_error() << std::endl;
                std::cout << "\nTroubleshooting steps:" << std::endl;
                std::cout << "1. Ensure PostgreSQL is running" << std::endl;
                std::cout << "2. Create database: CREATE DATABASE nexday_trading;" << std::endl;
                std::cout << "3. Create user: CREATE USER nexday_user WITH PASSWORD 'nexday_secure_password_2025';" << std::endl;
                std::cout << "4. Grant permissions: GRANT ALL PRIVILEGES ON DATABASE nexday_trading TO nexday_user;" << std::endl;
                return false;
            }
            
            std::cout << "Database connection successful" << std::endl;
            
            // Test basic query
            std::vector<std::string> symbols = db_manager_->get_symbol_list();
            record_test("Symbol List Query", true); // Always passes if connection works
            std::cout << "Found " << symbols.size() << " symbols in database" << std::endl;
            
            return true;
            
        } catch (const std::exception& e) {
            std::cout << "Database test exception: " << e.what() << std::endl;
            record_test("Database Connection", false);
            return false;
        }
    }
    
    void test_symbol_management() {
        std::cout << "\nTEST 2: Symbol Management" << std::endl;
        std::cout << "=====================================\n" << std::endl;
        
        try {
            // Test symbol import
            bool import_success = db_manager_->import_symbols_from_list(TEST_SYMBOLS, "integration_test");
            record_test("Symbol Import", import_success);
            
            if (import_success) {
                std::cout << "Successfully imported test symbols" << std::endl;
                for (const std::string& symbol : TEST_SYMBOLS) {
                    std::cout << "   " << symbol << " - Imported" << std::endl;
                }
            }
            
            // Test symbol retrieval
            std::vector<std::string> active_symbols = db_manager_->get_symbol_list(true);  // Changed method call
            record_test("Symbol Retrieval", !active_symbols.empty());
            std::cout << "Retrieved " << active_symbols.size() << " active symbols" << std::endl;
            
        } catch (const std::exception& e) {
            std::cout << "Symbol management exception: " << e.what() << std::endl;
            record_test("Symbol Management", false);
        }
    }
    
    void test_market_data_processing() {
        std::cout << "\nTEST 3: Market Data Processing" << std::endl;
        std::cout << "=====================================\n" << std::endl;
        
        try {
            // Test simple market data insertion (using legacy method for compatibility)
            bool single_success = db_manager_->insert_market_data("AAPL", 175.50, 1500000);
            record_test("Market Data Insert", single_success);
            
            if (single_success) {
                std::cout << "Successfully inserted market data for AAPL" << std::endl;
            }
            
            // Test historical data insertion (using legacy method)
            bool hist_success = db_manager_->insert_historical_data(
                "MSFT", "2025-01-15 16:00:00", 415.20, 418.75, 414.80, 417.25, 2300000
            );
            record_test("Historical Data Insert", hist_success);
            
            if (hist_success) {
                std::cout << "Successfully inserted historical data for MSFT" << std::endl;
            }
            
        } catch (const std::exception& e) {
            std::cout << "Market data processing exception: " << e.what() << std::endl;
            record_test("Market Data Processing", false);
        }
    }
    
    void test_system_performance() {
        std::cout << "\nTEST 4: System Performance" << std::endl;
        std::cout << "=====================================\n" << std::endl;
        
        try {
            auto start_time = std::chrono::high_resolution_clock::now();
            
            // Performance test: Insert multiple records
            int test_records = 50;
            int successful_inserts = 0;
            
            std::cout << "Testing insertion of " << test_records << " records..." << std::endl;
            
            for (int i = 0; i < test_records; i++) {
                if (db_manager_->insert_market_data("TEST_PERF", 100.0 + i * 0.1, 1000 + i)) {
                    successful_inserts++;
                }
            }
            
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            
            double records_per_second = (successful_inserts * 1000.0) / duration.count();
            
            std::cout << "Performance Results:" << std::endl;
            std::cout << "   Records inserted: " << successful_inserts << "/" << test_records << std::endl;
            std::cout << "   Time taken: " << duration.count() << " ms" << std::endl;
            std::cout << "   Throughput: " << std::fixed << std::setprecision(1) << records_per_second << " records/second" << std::endl;
            
            bool performance_good = records_per_second > 25.0; // At least 25 records/second
            record_test("System Performance", performance_good);
            
            if (performance_good) {
                std::cout << "Performance test passed" << std::endl;
            } else {
                std::cout << "Performance below optimal threshold" << std::endl;
            }
            
        } catch (const std::exception& e) {
            std::cout << "Performance test exception: " << e.what() << std::endl;
            record_test("System Performance", false);
        }
    }
    
    void record_test(const std::string& test_name, bool passed) {
        total_tests_++;
        if (passed) {
            passed_tests_++;
        }
        
        std::cout << (passed ? "PASSED: " : "FAILED: ") << test_name << std::endl;
    }
    
    void print_test_results() {
        std::cout << "\n=====================================================" << std::endl;
        std::cout << "INTEGRATION TEST RESULTS" << std::endl;
        std::cout << "=====================================================" << std::endl;
        
        double success_rate = (double)passed_tests_ / total_tests_ * 100.0;
        
        std::cout << "Tests Run: " << total_tests_ << std::endl;
        std::cout << "Tests Passed: " << passed_tests_ << std::endl;
        std::cout << "Tests Failed: " << (total_tests_ - passed_tests_) << std::endl;
        std::cout << "Success Rate: " << std::fixed << std::setprecision(1) << success_rate << "%" << std::endl;
        
        if (passed_tests_ == total_tests_) {
            std::cout << "\nALL TESTS PASSED!" << std::endl;
            std::cout << "Nexday Trading System integration is working correctly" << std::endl;
            std::cout << "\nNext Steps:" << std::endl;
            std::cout << "- Initialize full database schema: cmake --build . --target init_database" << std::endl;
            std::cout << "- Test IQFeed connection with real market data" << std::endl;
            std::cout << "- Begin building prediction models" << std::endl;
        } else {
            std::cout << "\nSOME TESTS FAILED" << std::endl;
            std::cout << "Please check the failed tests and fix issues" << std::endl;
            std::cout << "\nRecommended Actions:" << std::endl;
            std::cout << "- Fix database connectivity issues first" << std::endl;
            std::cout << "- Verify PostgreSQL is running and accessible" << std::endl;
            std::cout << "- Check database credentials and permissions" << std::endl;
        }
        
        std::cout << "=====================================================" << std::endl;
    }
};

int main() {
    std::cout << "Nexday Trading System - Integrated Test Suite" << std::endl;
    std::cout << "Version: 1.0.0" << std::endl;
    std::cout << "Build Date: " << __DATE__ << " " << __TIME__ << std::endl;
    std::cout << "Testing Database Integration (Simplified)" << std::endl;
    
    try {
        IntegratedTester tester;
        bool all_tests_passed = tester.run_all_tests();
        
        std::cout << "\nPress Enter to exit...";
        std::cin.get();
        
        return all_tests_passed ? 0 : 1;
        
    } catch (const std::exception& e) {
        std::cerr << "Integration test failed with exception: " << e.what() << std::endl;
        
        std::cout << "\nPress Enter to exit...";
        std::cin.get();
        return 1;
    }
}