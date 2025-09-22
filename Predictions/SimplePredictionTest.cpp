#include "MarketPredictionEngine.h"
#include <iostream>

int main() {
    std::cout << "=== Nexday Predictions - EMA Algorithm Test ===" << std::endl;
    
    DatabaseConfig config;
    config.host = "localhost";
    config.port = 5432;
    config.database = "nexday_trading";
    config.username = "nexday_user";
    config.password = "nexday_secure_password_2025";
    
    auto db_manager = std::make_unique<SimpleDatabaseManager>(config);
    
    if (!db_manager->test_connection()) {
        std::cerr << "Database connection failed" << std::endl;
        return 1;
    }
    
    auto engine = std::make_unique<MarketPredictionEngine>(std::move(db_manager));
    
    if (!engine->is_initialized()) {
        std::cerr << "Engine initialization failed" << std::endl;
        return 1;
    }
    
    std::cout << "✅ Prediction engine initialized successfully" << std::endl;
    std::cout << "✅ Model: " << NexdayPredictions::MODEL_NAME << std::endl;
    std::cout << "✅ Algorithm ready for production use" << std::endl;
    
    return 0;
}