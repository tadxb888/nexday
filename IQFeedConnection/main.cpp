#include <iostream>
#include <memory>
#include <string>
#include <chrono>
#include <thread>

#include "IQFeedConnectionManager.h"
#include "DailyDataFetcher.h"
#include "FifteenMinDataFetcher.h"
#include "ThirtyMinDataFetcher.h"
#include "OneHourDataFetcher.h"
#include "TwoHourDataFetcher.h"  // Changed from FourHourDataFetcher.h

int main() {
    std::cout << "\n==============================================\n";
    std::cout << "   NEXDAY TRADING - IQFeed Modular System\n";
    std::cout << "==============================================\n";
    
    try {
        // Create connection manager
        auto connection_manager = std::make_shared<IQFeedConnectionManager>();
        
        // Initialize connection to IQFeed
        if (!connection_manager->initialize_connection()) {
            std::cerr << "❌ Failed to initialize IQFeed connection" << std::endl;
            return 1;
        }
        
        std::cout << "✅ IQFeed connection established successfully\n" << std::endl;
        
        // Create data fetchers for different timeframes
        auto daily_fetcher = std::make_unique<DailyDataFetcher>(connection_manager);
        auto fifteen_min_fetcher = std::make_unique<FifteenMinDataFetcher>(connection_manager);
        auto thirty_min_fetcher = std::make_unique<ThirtyMinDataFetcher>(connection_manager);
        auto one_hour_fetcher = std::make_unique<OneHourDataFetcher>(connection_manager);
        auto two_hour_fetcher = std::make_unique<TwoHourDataFetcher>(connection_manager);  // Changed from four_hour_fetcher
        
        // Test symbol
        std::string symbol = "QGC#";  // Gold futures
        int num_bars = 20;
        
        std::cout << "Testing historical data fetching for symbol: " << symbol << "\n" << std::endl;
        
        // Test Daily Data
        std::cout << "📊 Testing Daily Data Fetcher...\n";
        std::vector<HistoricalBar> daily_data;
        if (daily_fetcher->fetch_historical_data(symbol, num_bars, daily_data)) {
            daily_fetcher->display_historical_data(symbol, daily_data);
        } else {
            std::cerr << "❌ Failed to fetch daily data" << std::endl;
        }
        
        std::cout << "\n" << std::string(50, '=') << "\n";
        
        // Test 15-Minute Data
        std::cout << "📊 Testing 15-Minute Data Fetcher...\n";
        std::vector<HistoricalBar> fifteen_min_data;
        if (fifteen_min_fetcher->fetch_historical_data(symbol, num_bars, fifteen_min_data)) {
            fifteen_min_fetcher->display_historical_data(symbol, fifteen_min_data);
        } else {
            std::cerr << "❌ Failed to fetch 15-minute data" << std::endl;
        }
        
        std::cout << "\n" << std::string(50, '=') << "\n";
        
        // Test 30-Minute Data
        std::cout << "📊 Testing 30-Minute Data Fetcher...\n";
        std::vector<HistoricalBar> thirty_min_data;
        if (thirty_min_fetcher->fetch_historical_data(symbol, num_bars, thirty_min_data)) {
            thirty_min_fetcher->display_historical_data(symbol, thirty_min_data);
        } else {
            std::cerr << "❌ Failed to fetch 30-minute data" << std::endl;
        }
        
        std::cout << "\n" << std::string(50, '=') << "\n";
        
        // Test 1-Hour Data
        std::cout << "📊 Testing 1-Hour Data Fetcher...\n";
        std::vector<HistoricalBar> one_hour_data;
        if (one_hour_fetcher->fetch_historical_data(symbol, num_bars, one_hour_data)) {
            one_hour_fetcher->display_historical_data(symbol, one_hour_data);
        } else {
            std::cerr << "❌ Failed to fetch 1-hour data" << std::endl;
        }
        
        std::cout << "\n" << std::string(50, '=') << "\n";
        
        // Test 2-Hour Data (replacing 4-hour)
        std::cout << "📊 Testing 2-Hour Data Fetcher...\n";
        std::vector<HistoricalBar> two_hour_data;
        if (two_hour_fetcher->fetch_historical_data(symbol, num_bars, two_hour_data)) {
            two_hour_fetcher->display_historical_data(symbol, two_hour_data);
        } else {
            std::cerr << "❌ Failed to fetch 2-hour data" << std::endl;
        }
        
        std::cout << "\n==============================================\n";
        std::cout << "✅ All historical data fetchers tested successfully!\n";
        std::cout << "==============================================\n";
        
        // Keep connection alive for a moment
        std::cout << "\nConnection will close in 3 seconds...\n";
        std::this_thread::sleep_for(std::chrono::seconds(3));
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Exception occurred: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "🔌 Disconnecting from IQFeed...\n";
    std::cout << "👋 Goodbye!\n" << std::endl;
    
    return 0;
}