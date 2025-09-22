#pragma once

/**
 * @file NexdayMarketPredictions.h
 * @brief Nexday Markets - Epoch Market Advisor Prediction Engine
 * @version 1.0.0
 * @date 2025
 * 
 * NEXDAY MARKETS PREDICTION SYSTEM
 * =================================
 * 
 * This module implements the "Epoch Market Advisor" prediction engine using 
 * Model 1 Standard algorithms based on Exponential Moving Average (EMA) techniques.
 * 
 * CORE FEATURES:
 * - Daily OHLC predictions (next business day)
 * - Intraday High/Low predictions (15min, 30min, 1hour, 2hour)
 * - Business day logic (Friday → Monday predictions)
 * - EMA bootstrap process with SMA foundation
 * - PostgreSQL integration with comprehensive error handling
 * 
 * PREDICTION ALGORITHM:
 * 1. Bootstrap Process: Calculate SMA1-SMA10 using 5-bar rolling windows
 * 2. EMA Initialization: Use SMA10 as initial previous_predict
 * 3. EMA Sequence: Apply formula predict_t = (0.5 * current_value) + (0.5 * previous_predict)
 * 4. Continuous Chain: Each EMA feeds into the next (EMA11 → EMA12 → ... → EMA_final)
 * 5. Final Prediction: EMA_final becomes the predicted value for next interval
 */

// Core prediction system headers
#include "PredictionTypes.h"
#include "BusinessDayCalculator.h" 
#include "MarketPredictionEngine.h"
#include "database_simple.h"

// Standard library headers
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <chrono>

// ==============================================
// SYSTEM CONSTANTS AND CONFIGURATION
// ==============================================

namespace NexdayPredictions {
    
    // Model 1 Standard Parameters
    constexpr double BASE_ALPHA = 0.5;         // EMA smoothing factor
    constexpr int MINIMUM_BARS = 15;           // Minimum historical data required
    constexpr int SMA_PERIODS = 10;            // Bootstrap SMA calculations
    constexpr int SMA_WINDOW = 5;              // SMA rolling window size
    
    // System Information
    constexpr const char* MODEL_NAME = "Epoch Market Advisor";
    constexpr const char* MODEL_VERSION = "1.0";
    constexpr const char* ALGORITHM_TYPE = "technical_analysis";
    
    /**
     * @brief Quick initialization function for the prediction system
     * @param db_config Database configuration
     * @return Initialized MarketPredictionEngine or nullptr on failure
     */
    inline std::unique_ptr<MarketPredictionEngine> initialize_prediction_system(
        const DatabaseConfig& db_config) {
        
        try {
            auto db_manager = std::make_unique<SimpleDatabaseManager>(db_config);
            
            if (!db_manager->test_connection()) {
                std::cerr << "Database connection failed" << std::endl;
                return nullptr;
            }
            
            auto engine = std::make_unique<MarketPredictionEngine>(std::move(db_manager));
            
            if (!engine->is_initialized()) {
                std::cerr << "Prediction engine initialization failed" << std::endl;
                return nullptr;
            }
            
            return engine;
            
        } catch (const std::exception& e) {
            std::cerr << "Exception during system initialization: " << e.what() << std::endl;
            return nullptr;
        }
    }
    
    /**
     * @brief Default database configuration for typical setup
     * @return Configured DatabaseConfig with standard Nexday settings
     */
    inline DatabaseConfig get_default_database_config() {
        DatabaseConfig config;
        config.host = "localhost";
        config.port = 5432;
        config.database = "nexday_trading";
        config.username = "nexday_user";
        config.password = "nexday_secure_password_2025";
        return config;
    }
    
} // namespace NexdayPredictions

/**
 * INTEGRATION EXAMPLE:
 * ===================
 * 
 * #include "NexdayMarketPredictions.h"
 * using namespace NexdayPredictions;
 * 
 * int main() {
 *     // Initialize with default configuration
 *     auto config = get_default_database_config();
 *     auto engine = initialize_prediction_system(config);
 *     
 *     if (engine) {
 *         // Generate predictions for gold futures
 *         engine->generate_predictions_for_symbol("QGC#");
 *         
 *         // Generate predictions for all active symbols
 *         engine->generate_predictions_for_all_active_symbols();
 *     }
 *     
 *     return 0;
 * }
 * 
 * BUILD INSTRUCTIONS:
 * ===================
 * 
 * 1. mkdir build && cd build
 * 2. cmake .. -DCMAKE_BUILD_TYPE=Release
 * 3. make -j4
 * 4. ./NexdayPredictionManager
 */