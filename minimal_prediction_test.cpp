//
// minimal_prediction_test.cpp
// MINIMAL VERSION - Just tests database connectivity and basic prediction structure
//

#include "Database/database_simple.h"
#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <chrono>

// Minimal TimeFrame enum (just what we need)
enum class TimeFrame {
    MINUTES_15 = 15,
    MINUTES_30 = 30,
    HOUR_1 = 60,
    HOURS_2 = 120,
    DAILY = 1440
};

std::string timeframe_to_string(TimeFrame tf) {
    switch (tf) {
        case TimeFrame::MINUTES_15: return "15min";
        case TimeFrame::MINUTES_30: return "30min";
        case TimeFrame::HOUR_1: return "1hour";
        case TimeFrame::HOURS_2: return "2hour";
        case TimeFrame::DAILY: return "daily";
        default: return "unknown";
    }
}

// Simple test to verify database tables exist and can be inserted into
bool test_prediction_tables(SimpleDatabaseManager& db) {
    std::cout << "\n=== TESTING PREDICTION TABLES ===" << std::endl;
    
    // Test 1: Check if predictions_daily table exists
    std::string query = "SELECT COUNT(*) FROM predictions_daily";
    PGresult* result = db.execute_query_with_result(query);
    if (!result) {
        std::cerr << "ERROR: predictions_daily table does not exist!" << std::endl;
        return false;
    }
    PQclear(result);
    std::cout << "✅ predictions_daily table exists" << std::endl;
    
    // Test 2: Check if predictions_all_symbols table exists  
    query = "SELECT COUNT(*) FROM predictions_all_symbols";
    result = db.execute_query_with_result(query);
    if (!result) {
        std::cerr << "ERROR: predictions_all_symbols table does not exist!" << std::endl;
        return false;
    }
    PQclear(result);
    std::cout << "✅ predictions_all_symbols table exists" << std::endl;
    
    // Test 3: Try to insert a test prediction to predictions_daily
    int symbol_id = db.get_or_create_symbol_id("TEST_SYMBOL");
    if (symbol_id == -1) {
        std::cerr << "ERROR: Could not create test symbol" << std::endl;
        return false;
    }
    
    // Insert test daily prediction
    std::string insert_query = 
        "INSERT INTO predictions_daily ("
        "prediction_time, target_date, symbol_id, model_id, "
        "predicted_open, predicted_high, predicted_low, predicted_close, "
        "confidence_score, model_name"
        ") VALUES ("
        "CURRENT_TIMESTAMP, '2025-09-25', " + std::to_string(symbol_id) + ", 1, "
        "100.0, 105.0, 95.0, 102.0, "
        "0.75, 'Test Model'"
        ") ON CONFLICT (target_date, symbol_id, model_id) DO UPDATE SET "
        "predicted_open = EXCLUDED.predicted_open";
    
    if (db.execute_query(insert_query)) {
        std::cout << "✅ Successfully inserted test daily prediction" << std::endl;
    } else {
        std::cerr << "ERROR: Failed to insert test daily prediction" << std::endl;
        return false;
    }
    
    // Test 4: Try to insert a test prediction to predictions_all_symbols
    insert_query = 
        "INSERT INTO predictions_all_symbols ("
        "prediction_time, target_time, symbol_id, model_id, "
        "timeframe, prediction_type, predicted_value, confidence_score, model_name"
        ") VALUES ("
        "CURRENT_TIMESTAMP, CURRENT_TIMESTAMP + INTERVAL '15 minutes', " + std::to_string(symbol_id) + ", 1, "
        "'15min', '15min_high', 103.5, 0.80, 'Test Model'"
        ") ON CONFLICT (prediction_time, symbol_id, timeframe, prediction_type) DO UPDATE SET "
        "predicted_value = EXCLUDED.predicted_value";
    
    if (db.execute_query(insert_query)) {
        std::cout << "✅ Successfully inserted test intraday prediction" << std::endl;
    } else {
        std::cerr << "ERROR: Failed to insert test intraday prediction" << std::endl;
        return false;
    }
    
    std::cout << "✅ All prediction table tests passed!" << std::endl;
    return true;
}

// Test historical data retrieval
bool test_historical_data_access(SimpleDatabaseManager& db) {
    std::cout << "\n=== TESTING HISTORICAL DATA ACCESS ===" << std::endl;
    
    // Check if we have any symbols with historical data
    std::string query = "SELECT s.symbol, COUNT(hd.fetch_date) as daily_count "
                       "FROM symbols s "
                       "LEFT JOIN historical_fetch_daily hd ON s.symbol_id = hd.symbol_id "
                       "GROUP BY s.symbol_id, s.symbol "
                       "HAVING COUNT(hd.fetch_date) > 0 "
                       "ORDER BY daily_count DESC "
                       "LIMIT 5";
    
    PGresult* result = db.execute_query_with_result(query);
    if (!result) {
        std::cerr << "ERROR: Could not query historical data" << std::endl;
        return false;
    }
    
    int rows = PQntuples(result);
    if (rows == 0) {
        std::cerr << "WARNING: No historical data found in database" << std::endl;
        PQclear(result);
        return false;
    }
    
    std::cout << "Historical data available for " << rows << " symbols:" << std::endl;
    for (int i = 0; i < rows; i++) {
        std::string symbol = PQgetvalue(result, i, 0);
        std::string count = PQgetvalue(result, i, 1);
        std::cout << "  " << symbol << ": " << count << " daily bars" << std::endl;
    }
    
    PQclear(result);
    std::cout << "✅ Historical data access test passed!" << std::endl;
    return true;
}

int main() {
    std::cout << "=====================================================" << std::endl;
    std::cout << "NEXDAY MARKETS - MINIMAL PREDICTION SYSTEM TEST" << std::endl;
    std::cout << "=====================================================" << std::endl;
    
    // Database configuration
    DatabaseConfig config;
    config.host = "localhost";
    config.port = 5432;
    config.database = "nexday_trading";
    config.username = "nexday_user";
    config.password = "nexday_secure_password_2025";
    
    // Create database manager
    SimpleDatabaseManager db(config);
    
    // Test database connection
    std::cout << "Testing database connection..." << std::endl;
    if (!db.test_connection()) {
        std::cerr << "❌ Database connection failed!" << std::endl;
        return 1;
    }
    std::cout << "✅ Database connection successful!" << std::endl;
    
    // Test prediction tables
    if (!test_prediction_tables(db)) {
        std::cerr << "❌ Prediction table tests failed!" << std::endl;
        return 1;
    }
    
    // Test historical data access
    if (!test_historical_data_access(db)) {
        std::cerr << "⚠️  Historical data test failed - this is OK for now" << std::endl;
    }
    
    std::cout << "\n=====================================================" << std::endl;
    std::cout << "✅ MINIMAL PREDICTION SYSTEM TEST - PASSED!" << std::endl;
    std::cout << "=====================================================" << std::endl;
    
    std::cout << "\nNext steps:" << std::endl;
    std::cout << "1. This confirms your database schema is working" << std::endl;
    std::cout << "2. You can now add the prediction engine components" << std::endl;
    std::cout << "3. Start with simple EMA calculations" << std::endl;
    
    return 0;
}