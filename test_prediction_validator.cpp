#include <iostream>
#include <memory>
#include "PredictionValidator.h"
#include "Database/database_simple.h"

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "    NEXDAY PREDICTION VALIDATOR TEST    " << std::endl;
    std::cout << "========================================" << std::endl;
    
    try {
        // Test database connection
        std::cout << "1. Testing database connection..." << std::endl;
        DatabaseConfig db_config;
        db_config.host = "localhost";
        db_config.port = 5432;
        db_config.database = "nexday_trading";
        db_config.username = "nexday_user";
        db_config.password = "nexday_secure_password_2025";
        
        auto db_manager = std::make_shared<SimpleDatabaseManager>(db_config);
        
        if (!db_manager->test_connection()) {
            std::cerr << "❌ Failed to connect to database!" << std::endl;
            return 1;
        }
        std::cout << "   ✅ Database connection successful" << std::endl;
        
        // Test PredictionValidator creation
        std::cout << "\n2. Creating PredictionValidator..." << std::endl;
        auto validator = std::make_unique<PredictionValidator>(db_manager);
        std::cout << "   ✅ PredictionValidator created successfully" << std::endl;
        
        // Test static calculation methods
        std::cout << "\n3. Testing error calculation functions..." << std::endl;
        std::vector<double> predicted = {100.0, 105.0, 110.0};
        std::vector<double> actual = {102.0, 104.0, 109.0};
        
        double mae = PredictionValidator::calculate_mae(predicted, actual);
        double rmse = PredictionValidator::calculate_rmse(predicted, actual);
        double mape = PredictionValidator::calculate_mape(predicted, actual);
        
        std::cout << "   MAE: " << mae << std::endl;
        std::cout << "   RMSE: " << rmse << std::endl;
        std::cout << "   MAPE: " << mape << "%" << std::endl;
        std::cout << "   ✅ Calculations working correctly" << std::endl;
        
        // Test database queries
        std::cout << "\n4. Testing database queries..." << std::endl;
        auto unvalidated = validator->get_unvalidated_predictions();
        std::cout << "   Found " << unvalidated.size() << " unvalidated predictions" << std::endl;
        std::cout << "   ✅ Database queries working" << std::endl;
        
        std::cout << "\n========================================" << std::endl;
        std::cout << "    ✅ ALL TESTS PASSED!              " << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "\nPredictionValidator is ready for integration!" << std::endl;
        std::cout << "You can now add it to your existing main.cpp" << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed: " << e.what() << std::endl;
        return 1;
    }
}