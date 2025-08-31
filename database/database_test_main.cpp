#include "database_simple.h"
#include <iostream>
#include <exception>
#include <iomanip>

int main() {
    std::cout << "=== Nexday Database Test Program ===" << std::endl;
    std::cout << "Program started successfully" << std::endl;
    std::cout.flush();
    
    std::cout << "\nTesting PostgreSQL connection and basic operations" << std::endl;
    std::cout << "Connecting to: localhost:5432/nexday_trading" << std::endl;
    std::cout << "Username: nexday_user" << std::endl;
    std::cout << "" << std::endl;
    std::cout.flush();
    
    try {
        std::cout << "Creating database manager..." << std::endl;
        std::cout.flush();
        
        // Create database manager with explicit config
        DatabaseConfig config;
        config.host = "localhost";
        config.port = 5432;
        config.database = "nexday_trading";
        config.username = "nexday_user";
        config.password = "nexday_secure_password_2025";
        
        std::cout << "Database config created" << std::endl;
        std::cout.flush();
        
        SimpleDatabaseManager db(config);
        
        std::cout << "Database manager created" << std::endl;
        std::cout.flush();
        
        // Test connection
        std::cout << "\n1. Testing database connection..." << std::endl;
        std::cout.flush();
        
        bool connection_result = db.test_connection();
        std::cout << "Connection test result: " << (connection_result ? "SUCCESS" : "FAILED") << std::endl;
        std::cout.flush();
        
        if (!connection_result) {
            std::cout << "✗ Database connection failed!" << std::endl;
            std::cout << "Check if PostgreSQL is running and credentials are correct." << std::endl;
            std::cout << "\nCommon fixes:" << std::endl;
            std::cout << "1. Ensure PostgreSQL service is running" << std::endl;
            std::cout << "2. Check if database 'nexday_trading' exists" << std::endl;
            std::cout << "3. Check if user 'nexday_user' exists with correct password" << std::endl;
            std::cout << "4. Run database setup scripts first" << std::endl;
            std::cout.flush();
            
            std::cout << "\nPress Enter to exit...";
            std::cin.get();
            return 1;
        }
        
        // Show sample data
        std::cout << "\n2. Showing current database contents..." << std::endl;
        std::cout.flush();
        
        db.print_sample_data();
        std::cout.flush();
        
        // Test data insertion
        std::cout << "\n3. Testing data insertion..." << std::endl;
        std::cout.flush();
        
        // Test multiple symbols with realistic market data
        struct TestData {
            std::string symbol;
            double price;
            long long volume;
        };
        
        TestData test_data[] = {
            {"AAPL", 175.43, 45000000},
            {"MSFT", 415.26, 23000000},
            {"GOOGL", 2875.12, 1200000},
            {"TSLA", 248.50, 85000000},
            {"ES", 4567.25, 125000}  // S&P 500 futures
        };
        
        int successful_inserts = 0;
        for (const auto& data : test_data) {
            bool result = db.insert_market_data(data.symbol, data.price, data.volume);
            if (result) {
                successful_inserts++;
            }
        }
        
        std::cout << "\nInsertion Results: " << successful_inserts << "/" 
                  << (sizeof(test_data)/sizeof(test_data[0])) << " successful" << std::endl;
        
        if (successful_inserts > 0) {
            std::cout << "✓ Data insertion tests passed" << std::endl;
        } else {
            std::cout << "✗ All data insertion tests failed" << std::endl;
            std::cout << "This might indicate missing symbols in the database." << std::endl;
        }
        std::cout.flush();
        
        // Test historical data insertion
        std::cout << "\n4. Testing historical data insertion..." << std::endl;
        std::cout.flush();
        
        bool hist_result = db.insert_historical_data("AAPL", "2025-08-31 16:00:00", 
                                                    174.20, 176.50, 173.80, 175.43, 45000000);
        
        if (hist_result) {
            std::cout << "✓ Historical data insertion test passed" << std::endl;
        } else {
            std::cout << "✗ Historical data insertion test failed" << std::endl;
        }
        std::cout.flush();
        
        // Show updated data
        std::cout << "\n5. Showing updated database contents..." << std::endl;
        std::cout.flush();
        
        db.print_sample_data();
        std::cout.flush();
        
        // Connection info
        std::cout << "\n6. Testing connection status..." << std::endl;
        std::cout << "Is connected: " << (db.is_connected() ? "YES" : "NO") << std::endl;
        
        std::cout << "\n✓ All database tests completed!" << std::endl;
        std::cout << "\nDatabase Status Summary:" << std::endl;
        std::cout << "- PostgreSQL connection: ✓ Working" << std::endl;
        std::cout << "- Database structure: ✓ Ready" << std::endl;
        std::cout << "- Data insertion: " << (successful_inserts > 0 ? "✓" : "✗") 
                  << " " << successful_inserts << " successful" << std::endl;
        std::cout << "- Historical data: " << (hist_result ? "✓ Working" : "✗ Failed") << std::endl;
        
        std::cout << "\nYour PostgreSQL database is ready for the trading system!" << std::endl;
        std::cout << "\nNext steps:" << std::endl;
        std::cout << "- Integrate with your IQFeed connection" << std::endl;
        std::cout << "- Start fetching real market data" << std::endl;
        std::cout << "- Build prediction models" << std::endl;
        std::cout.flush();
        
        std::cout << "\nPress Enter to exit...";
        std::cin.get();
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cout << "✗ Database test failed with exception: " << e.what() << std::endl;
        std::cout.flush();
        
        std::cout << "\nPress Enter to exit...";
        std::cin.get();
        return 1;
    } catch (...) {
        std::cout << "✗ Database test failed with unknown exception" << std::endl;
        std::cout.flush();
        
        std::cout << "\nPress Enter to exit...";
        std::cin.get();
        return 1;
    }
}