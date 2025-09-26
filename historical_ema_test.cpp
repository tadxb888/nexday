#include "Database/database_simple.h"
#include <iostream>
#include <vector>
#include <algorithm>

// Your working EMA calculator (from ema_test.cpp)
class SimpleEMACalculator {
public:
    static double calculate_sma(const std::vector<double>& values, int start_index, int window_size) {
        if (start_index + window_size > values.size()) return 0.0;
        double sum = 0.0;
        for (int i = start_index; i < start_index + window_size; i++) {
            sum += values[i];
        }
        return sum / window_size;
    }
    
    static std::vector<double> calculate_ema_sequence(const std::vector<double>& values, double initial_previous_predict) {
        std::vector<double> ema_values;
        double previous_predict = initial_previous_predict;
        const double BASE_ALPHA = 0.5;
        
        for (double current_value : values) {
            double predict_t = (BASE_ALPHA * current_value) + ((1.0 - BASE_ALPHA) * previous_predict);
            ema_values.push_back(predict_t);
            previous_predict = predict_t;
        }
        return ema_values;
    }
};

// Get real historical data from your database
std::vector<double> get_real_close_prices(SimpleDatabaseManager& db, const std::string& symbol, int limit = 25) {
    std::vector<double> prices;
    
    int symbol_id = db.get_symbol_id(symbol);
    if (symbol_id == -1) {
        std::cout << "Symbol " << symbol << " not found" << std::endl;
        return prices;
    }
    
    std::string query = "SELECT close_price FROM historical_fetch_daily WHERE symbol_id = " + 
                       std::to_string(symbol_id) + " ORDER BY fetch_date ASC LIMIT " + std::to_string(limit);
    
    PGresult* result = db.execute_query_with_result(query);
    if (!result) {
        std::cout << "No historical data found for " << symbol << std::endl;
        return prices;
    }
    
    int rows = PQntuples(result);
    for (int i = 0; i < rows; i++) {
        double price = std::stod(PQgetvalue(result, i, 0));
        prices.push_back(price);
    }
    
    PQclear(result);
    return prices;
}

int main() {
    std::cout << "=== HISTORICAL EMA TEST WITH REAL DATA ===" << std::endl;
    
    // Database setup (your working connection)
    DatabaseConfig config;
    config.host = "localhost";
    config.port = 5432;
    config.database = "nexday_trading";
    config.username = "postgres";
    config.password = "magical.521";  // Updated with correct password
    
    SimpleDatabaseManager db(config);
    if (!db.test_connection()) {
        std::cerr << "Database connection failed!" << std::endl;
        return 1;
    }
    
    std::cout << "Checking available symbols and their data counts..." << std::endl;

    // Get symbols with their data counts - Enhanced version
    std::string symbols_query = 
        "SELECT s.symbol, COUNT(hd.fetch_date) as bar_count "
        "FROM symbols s "
        "JOIN historical_fetch_daily hd ON s.symbol_id = hd.symbol_id "
        "GROUP BY s.symbol_id, s.symbol "
        "HAVING COUNT(hd.fetch_date) >= 15 "
        "ORDER BY bar_count DESC "
        "LIMIT 5";

    PGresult* symbols_result = db.execute_query_with_result(symbols_query);
    if (!symbols_result || PQntuples(symbols_result) == 0) {
        std::cout << "No symbols with sufficient historical data (15+ bars) found" << std::endl;
        
        // Show what we do have
        std::string fallback_query = 
            "SELECT s.symbol, COUNT(hd.fetch_date) as bar_count "
            "FROM symbols s "
            "JOIN historical_fetch_daily hd ON s.symbol_id = hd.symbol_id "
            "GROUP BY s.symbol_id, s.symbol "
            "ORDER BY bar_count DESC "
            "LIMIT 10";
        
        PGresult* fallback_result = db.execute_query_with_result(fallback_query);
        if (fallback_result) {
            std::cout << "Available symbols and their bar counts:" << std::endl;
            int rows = PQntuples(fallback_result);
            for (int i = 0; i < rows; i++) {
                std::string symbol = PQgetvalue(fallback_result, i, 0);
                std::string count = PQgetvalue(fallback_result, i, 1);
                std::cout << "  " << symbol << ": " << count << " bars" << std::endl;
            }
            PQclear(fallback_result);
        }
        
        // Try with reduced minimum if no symbols have 15+ bars
        std::cout << "\nTrying with reduced minimum (5+ bars)..." << std::endl;
        std::string reduced_query = 
            "SELECT s.symbol, COUNT(hd.fetch_date) as bar_count "
            "FROM symbols s "
            "JOIN historical_fetch_daily hd ON s.symbol_id = hd.symbol_id "
            "GROUP BY s.symbol_id, s.symbol "
            "HAVING COUNT(hd.fetch_date) >= 5 "
            "ORDER BY bar_count DESC "
            "LIMIT 3";
            
        PGresult* reduced_result = db.execute_query_with_result(reduced_query);
        if (!reduced_result || PQntuples(reduced_result) == 0) {
            std::cout << "No symbols with even 5+ bars found" << std::endl;
            return 1;
        }
        
        symbols_result = reduced_result;
        std::cout << "Using reduced minimum for testing..." << std::endl;
    }

    // Show available symbols with enough data
    int symbol_count = PQntuples(symbols_result);
    std::cout << "Found " << symbol_count << " symbols with sufficient data:" << std::endl;
    for (int i = 0; i < symbol_count; i++) {
        std::string symbol = PQgetvalue(symbols_result, i, 0);
        std::string count = PQgetvalue(symbols_result, i, 1);
        std::cout << "  " << symbol << ": " << count << " bars" << std::endl;
    }

    // Use the symbol with the most data
    std::string test_symbol = PQgetvalue(symbols_result, 0, 0);
    std::string bar_count = PQgetvalue(symbols_result, 0, 1);
    PQclear(symbols_result);

    std::cout << "\nTesting with symbol: " << test_symbol << " (" << bar_count << " bars)" << std::endl;
    
    // Get real historical data
    auto real_prices = get_real_close_prices(db, test_symbol, 25);
    if (real_prices.empty()) {
        std::cout << "Could not retrieve historical data for " << test_symbol << std::endl;
        return 1;
    }
    
    std::cout << "Retrieved " << real_prices.size() << " historical bars" << std::endl;
    std::cout << "Price range: " << real_prices.front() << " to " << real_prices.back() << std::endl;
    
    // Show first few prices for verification
    std::cout << "First 5 prices: ";
    for (size_t i = 0; i < std::min(size_t(5), real_prices.size()); i++) {
        std::cout << real_prices[i] << " ";
    }
    std::cout << std::endl;
    
    // Check if we have enough data for full EMA calculation
    if (real_prices.size() < 15) {
        std::cout << "Warning: Only " << real_prices.size() << " bars available, need 15+ for full EMA calculation" << std::endl;
        std::cout << "Proceeding with simplified calculation..." << std::endl;
        
        // Use all available data with adjusted parameters
        int sma_periods = std::min(int(real_prices.size()) - 5, 10);
        if (sma_periods < 1) {
            std::cout << "Insufficient data for any calculation" << std::endl;
            return 1;
        }
        
        std::vector<double> sma_values;
        for (int i = 0; i < sma_periods; i++) {
            double sma = SimpleEMACalculator::calculate_sma(real_prices, i, std::min(5, int(real_prices.size()) - i));
            sma_values.push_back(sma);
        }
        
        double initial_previous_predict = sma_values.back();
        std::vector<double> ema_input_series(real_prices.begin() + sma_periods + 4, real_prices.end());
        
        if (!ema_input_series.empty()) {
            auto ema_values = SimpleEMACalculator::calculate_ema_sequence(ema_input_series, initial_previous_predict);
            double prediction = ema_values.back();
            std::cout << "\nSimplified EMA prediction for " << test_symbol << ": " << prediction << std::endl;
        } else {
            std::cout << "Not enough data for EMA sequence calculation" << std::endl;
        }
    } else {
        // Full EMA calculation with 15+ bars
        std::cout << "Performing full EMA calculation..." << std::endl;
        
        // Apply your working EMA calculation to real data
        std::vector<double> sma_values;
        for (int i = 0; i < 10; i++) {
            double sma = SimpleEMACalculator::calculate_sma(real_prices, i, 5);
            sma_values.push_back(sma);
            std::cout << "SMA" << (i+1) << ": " << sma << std::endl;
        }
        
        double initial_previous_predict = sma_values.back();
        std::cout << "\nUsing SMA10 as initial previous_predict: " << initial_previous_predict << std::endl;
        
        std::vector<double> ema_input_series(real_prices.begin() + 14, real_prices.end());
        auto ema_values = SimpleEMACalculator::calculate_ema_sequence(ema_input_series, initial_previous_predict);
        
        std::cout << "\nEMA sequence:" << std::endl;
        for (size_t i = 0; i < ema_values.size(); i++) {
            std::cout << "EMA" << (i + 15) << ": " << ema_values[i] << std::endl;
        }
        
        double prediction = ema_values.back();
        std::cout << "\nFull EMA prediction for " << test_symbol << ": " << prediction << std::endl;
    }
    
    std::cout << "✅ Historical EMA test with real data completed!" << std::endl;
    
    return 0;
}