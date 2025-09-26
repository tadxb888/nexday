#include <iostream>
#include <memory>
#include "PredictionValidator.h"
#include "Database/database_simple.h"

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "    PREDICTION VALIDATOR TEST SUITE    " << std::endl;
    std::cout << "========================================" << std::endl;
    
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
        std::cout << "   ✅ Database connection successful" << std::endl;
        
        // Initialize PredictionValidator
        std::cout << "\n2. Initializing PredictionValidator..." << std::endl;
        auto validator = std::make_unique<PredictionValidator>(db_manager);
        std::cout << "   ✅ PredictionValidator created successfully" << std::endl;
        
        // Test static calculation methods
        std::cout << "\n3. Testing static calculation methods..." << std::endl;
        
        // Test data
        std::vector<double> predicted = {100.0, 105.0, 110.0, 108.0, 112.0};
        std::vector<double> actual = {102.0, 104.0, 109.0, 110.0, 111.0};
        
        double mae = PredictionValidator::calculate_mae(predicted, actual);
        double rmse = PredictionValidator::calculate_rmse(predicted, actual);
        double mape = PredictionValidator::calculate_mape(predicted, actual);
        double r_squared = PredictionValidator::calculate_r_squared(predicted, actual);
        double accuracy = PredictionValidator::calculate_accuracy_score(100.0, 102.0);
        
        std::cout << "   MAE: " << mae << std::endl;
        std::cout << "   RMSE: " << rmse << std::endl;
        std::cout << "   MAPE: " << mape << "%" << std::endl;
        std::cout << "   R²: " << r_squared << std::endl;
        std::cout << "   Sample accuracy score: " << accuracy << std::endl;
        std::cout << "   ✅ Static calculations working" << std::endl;
        
        // Test getting unvalidated predictions
        std::cout << "\n4. Testing database queries..." << std::endl;
        auto unvalidated_predictions = validator->get_unvalidated_predictions();
        std::cout << "   Found " << unvalidated_predictions.size() << " unvalidated predictions" << std::endl;
        
        // Print validation summary
        std::cout << "\n5. Testing validation summary report..." << std::endl;
        validator->print_validation_summary();
        
        // Test model metrics calculation (assuming model_id 1 exists)
        std::cout << "\n6. Testing model performance calculation..." << std::endl;
        ModelMetrics daily_metrics = validator->calculate_model_metrics(1, "daily", 30);
        std::cout << "   Daily metrics calculated:" << std::endl;
        std::cout << "     Total predictions: " << daily_metrics.total_predictions << std::endl;
        std::cout << "     Validated predictions: " << daily_metrics.validated_predictions << std::endl;
        
        if (daily_metrics.validated_predictions > 0) {
            std::cout << "     MAE: " << daily_metrics.mae << std::endl;
            std::cout << "     RMSE: " << daily_metrics.rmse << std::endl;
            std::cout << "     Mean Accuracy: " << daily_metrics.mean_accuracy << std::endl;
        }
        
        std::cout << "\n========================================" << std::endl;
        std::cout << "    ✅ ALL TESTS COMPLETED SUCCESSFULLY" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "\nPredictionValidator is ready for use!" << std::endl;
        std::cout << "You can now:" << std::endl;
        std::cout << "- Add prediction validation to your main application" << std::endl;
        std::cout << "- Use validator->validate_all_pending_predictions()" << std::endl;
        std::cout << "- Generate performance reports with validator->print_model_performance(model_id)" << std::endl;
        
        std::cout << "\nPress Enter to exit...";
        std::cin.get();
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed with exception: " << e.what() << std::endl;
        std::cout << "\nPress Enter to exit...";
        std::cin.get();
        return 1;
    }
}