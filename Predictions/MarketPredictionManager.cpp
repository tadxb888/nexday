#include "MarketPredictionEngine.h"
#include "database_simple.h"
#include <iostream>
#include <memory>
#include <vector>
#include <string>

// ==============================================
// MARKET PREDICTION MANAGER - MAIN INTERFACE
// ==============================================

class MarketPredictionManager {
private:
    std::unique_ptr<MarketPredictionEngine> prediction_engine_;
    DatabaseConfig db_config_;
    bool is_initialized_;

public:
    MarketPredictionManager() : is_initialized_(false) {
        // Initialize database configuration
        db_config_.host = "localhost";
        db_config_.port = 5432;
        db_config_.database = "nexday_trading";
        db_config_.username = "nexday_user";
        db_config_.password = "nexday_secure_password_2025";
    }
    
    ~MarketPredictionManager() = default;
    
    // Initialize the prediction system
    bool initialize() {
        try {
            // Create database manager
            auto db_manager = std::make_unique<SimpleDatabaseManager>(db_config_);
            
            if (!db_manager->test_connection()) {
                std::cerr << "Failed to connect to database: " << db_manager->get_last_error() << std::endl;
                return false;
            }
            
            // Create prediction engine
            prediction_engine_ = std::make_unique<MarketPredictionEngine>(std::move(db_manager));
            
            if (!prediction_engine_->is_initialized()) {
                std::cerr << "Failed to initialize prediction engine: " << 
                            prediction_engine_->get_last_error() << std::endl;
                return false;
            }
            
            is_initialized_ = true;
            std::cout << "Market Prediction Manager initialized successfully" << std::endl;
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "Exception during initialization: " << e.what() << std::endl;
            return false;
        }
    }
    
    // Generate predictions for a specific symbol
    bool generate_predictions(const std::string& symbol) {
        if (!is_initialized_) {
            std::cerr << "Prediction manager not initialized" << std::endl;
            return false;
        }
        
        return prediction_engine_->generate_predictions_for_symbol(symbol);
    }
    
    // Generate predictions for all active symbols
    bool generate_all_predictions() {
        if (!is_initialized_) {
            std::cerr << "Prediction manager not initialized" << std::endl;
            return false;
        }
        
        return prediction_engine_->generate_predictions_for_all_active_symbols();
    }
    
    // Run prediction validation
    bool validate_model_performance(const std::string& symbol = "AAPL", int days = 30) {
        if (!is_initialized_) {
            std::cerr << "Prediction manager not initialized" << std::endl;
            return false;
        }
        
        std::cout << "\n=== MODEL VALIDATION REPORT ===" << std::endl;
        std::cout << "Symbol: " << symbol << std::endl;
        std::cout << "Validation Period: " << days << " days" << std::endl;
        
        // Validate different timeframes
        std::vector<TimeFrame> timeframes = {
            TimeFrame::DAILY,
            TimeFrame::HOUR_1,
            TimeFrame::MINUTES_30,
            TimeFrame::MINUTES_15
        };
        
        for (auto timeframe : timeframes) {
            auto validation = prediction_engine_->validate_predictions(symbol, timeframe, days);
            
            std::cout << "\n" << timeframe_to_string(timeframe) << " Predictions:" << std::endl;
            std::cout << "  Status: " << (validation.is_valid ? "VALID" : "INVALID") << std::endl;
            std::cout << "  MAE:    " << validation.mae << std::endl;
            std::cout << "  RMSE:   " << validation.rmse << std::endl;
            std::cout << "  MAPE:   " << validation.mape << "%" << std::endl;
            std::cout << "  R²:     " << validation.r2 << std::endl;
        }
        
        std::cout << "==============================\n" << std::endl;
        return true;
    }
    
    // Display system status
    void print_system_status() {
        std::cout << "\n=== NEXDAY MARKET PREDICTIONS STATUS ===" << std::endl;
        std::cout << "System Initialized: " << (is_initialized_ ? "YES" : "NO") << std::endl;
        
        if (is_initialized_) {
            std::cout << "Model: Epoch Market Advisor (Model 1 Standard)" << std::endl;
            std::cout << "Algorithm: Exponential Moving Average" << std::endl;
            std::cout << "Base Alpha: " << Model1Parameters::BASE_ALPHA << std::endl;
            std::cout << "Minimum Bars: " << Model1Parameters::MINIMUM_BARS << std::endl;
            std::cout << "Database: Connected" << std::endl;
            
            std::cout << "\nPrediction Types:" << std::endl;
            std::cout << "  • Daily OHLC (Next Business Day)" << std::endl;
            std::cout << "  • Intraday High/Low (15min, 30min, 1hour, 2hour)" << std::endl;
            
            std::cout << "\nBusiness Logic:" << std::endl;
            std::cout << "  • Weekend Skipping: Friday → Monday" << std::endl;
            std::cout << "  • EMA Bootstrap: 10 SMA calculations" << std::endl;
            std::cout << "  • Continuous Chain: EMA11 → EMA_final" << std::endl;
        }
        std::cout << "========================================\n" << std::endl;
    }
    
    // Test EMA calculation with sample data
    bool test_ema_calculation() {
        if (!is_initialized_) {
            std::cerr << "Prediction manager not initialized" << std::endl;
            return false;
        }
        
        std::cout << "\n=== EMA CALCULATION TEST ===" << std::endl;
        
        // Create sample historical data for testing
        std::vector<HistoricalBar> test_data;
        test_data.reserve(20);
        
        // Generate sample price data (ascending pattern for easy verification)
        auto base_time = std::chrono::system_clock::now();
        for (int i = 0; i < 20; i++) {
            HistoricalBar bar;
            bar.timestamp = base_time + std::chrono::hours(24 * i);
            bar.open = 100.0 + i;
            bar.high = 100.5 + i;
            bar.low = 99.5 + i;
            bar.close = 100.2 + i;
            bar.volume = 1000000;
            test_data.push_back(bar);
        }
        
        // Test EMA calculation
        auto ema_result = prediction_engine_->calculate_ema_for_prediction(test_data, "close");
        
        if (ema_result.valid) {
            prediction_engine_->print_ema_calculation_debug(test_data, ema_result);
            std::cout << "EMA calculation test: PASSED" << std::endl;
            return true;
        } else {
            std::cout << "EMA calculation test: FAILED" << std::endl;
            std::cout << "Error: " << prediction_engine_->get_last_error() << std::endl;
            return false;
        }
    }
    
    bool is_initialized() const { return is_initialized_; }
};

// ==============================================
// MAIN FUNCTION FOR TESTING
// ==============================================

int main() {
    std::cout << "=====================================================" << std::endl;
    std::cout << "NEXDAY MARKETS - EPOCH MARKET ADVISOR" << std::endl;
    std::cout << "Market Predictions Engine" << std::endl;
    std::cout << "=====================================================" << std::endl;
    
    // Create prediction manager
    MarketPredictionManager manager;
    
    // Initialize system
    if (!manager.initialize()) {
        std::cerr << "Failed to initialize prediction system" << std::endl;
        return 1;
    }
    
    // Display system status
    manager.print_system_status();
    
    // Test EMA calculation
    std::cout << "Testing EMA calculation algorithm..." << std::endl;
    if (!manager.test_ema_calculation()) {
        std::cerr << "EMA calculation test failed" << std::endl;
        return 1;
    }
    
    // Interactive menu
    int choice = 0;
    while (choice != 9) {
        std::cout << "\n=== MARKET PREDICTION MENU ===" << std::endl;
        std::cout << "1. Generate predictions for single symbol" << std::endl;
        std::cout << "2. Generate predictions for all symbols" << std::endl;
        std::cout << "3. Validate model performance" << std::endl;
        std::cout << "4. Test EMA calculation" << std::endl;
        std::cout << "5. Show system status" << std::endl;
        std::cout << "9. Exit" << std::endl;
        std::cout << "Choose option: ";
        
        std::cin >> choice;
        
        switch (choice) {
            case 1: {
                std::string symbol;
                std::cout << "Enter symbol (e.g., AAPL, QGC#): ";
                std::cin >> symbol;
                
                std::cout << "Generating predictions for " << symbol << "..." << std::endl;
                if (manager.generate_predictions(symbol)) {
                    std::cout << "Predictions generated successfully for " << symbol << std::endl;
                } else {
                    std::cout << "Failed to generate predictions for " << symbol << std::endl;
                }
                break;
            }
            
            case 2: {
                std::cout << "Generating predictions for all active symbols..." << std::endl;
                if (manager.generate_all_predictions()) {
                    std::cout << "All predictions generated successfully" << std::endl;
                } else {
                    std::cout << "Some predictions failed" << std::endl;
                }
                break;
            }
            
            case 3: {
                std::string symbol;
                std::cout << "Enter symbol for validation (default: AAPL): ";
                std::cin.ignore();
                std::getline(std::cin, symbol);
                if (symbol.empty()) symbol = "AAPL";
                
                manager.validate_model_performance(symbol);
                break;
            }
            
            case 4: {
                manager.test_ema_calculation();
                break;
            }
            
            case 5: {
                manager.print_system_status();
                break;
            }
            
            case 9: {
                std::cout << "Exiting Market Prediction Manager..." << std::endl;
                break;
            }
            
            default: {
                std::cout << "Invalid choice. Please try again." << std::endl;
                break;
            }
        }
    }
    
    std::cout << "\n=====================================================" << std::endl;
    std::cout << "NEXDAY MARKETS - PREDICTION ENGINE SHUTDOWN" << std::endl;
    std::cout << "=====================================================" << std::endl;
    
    return 0;
}