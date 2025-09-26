#ifndef PREDICTION_PERSISTER_H
#define PREDICTION_PERSISTER_H

#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include "../Database/database_simple.h"

// ==============================================
// OHLC PREDICTION STRUCTURE
// ==============================================

struct OHLCPrediction {
    std::string symbol;
    double predicted_open;
    double predicted_high; 
    double predicted_low;
    double predicted_close;
    std::string target_date;     // Date we're predicting for (YYYY-MM-DD)
    std::string prediction_time; // When prediction was made (YYYY-MM-DD HH:MM:SS)
    double confidence_score = 0.0;
};

// ==============================================
// PREDICTION PERSISTENCE UTILITIES
// ==============================================

class PredictionPersister {
public:
    // Get current timestamp in proper format
    static std::string get_current_timestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
    
    // Get next business day (handles weekend skipping)
    static std::string get_next_business_day() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto tm = *std::localtime(&time_t);
        
        // Add one day
        tm.tm_mday += 1;
        mktime(&tm);  // Normalize the date
        
        // Check if it's weekend and skip to Monday
        int day_of_week = tm.tm_wday;
        
        if (day_of_week == 0) {  // Sunday -> Monday
            tm.tm_mday += 1;
            mktime(&tm);
        } else if (day_of_week == 6) {  // Saturday -> Monday
            tm.tm_mday += 2;
            mktime(&tm);
        }
        
        std::stringstream ss;
        ss << std::put_time(&tm, "%Y-%m-%d");
        return ss.str();
    }
    
    // Save daily OHLC prediction to predictions_daily table (ACTUAL SCHEMA)
    static bool save_daily_prediction(SimpleDatabaseManager& db_manager, const OHLCPrediction& prediction) {
        std::cout << "💾 Saving daily OHLC prediction for " << prediction.symbol << std::endl;
        
        try {
            // Get symbol_id
            int symbol_id = db_manager.get_symbol_id(prediction.symbol);
            if (symbol_id <= 0) {
                std::cout << "❌ Could not find symbol ID for " << prediction.symbol << std::endl;
                return false;
            }
            
            // Insert into predictions_daily table using ACTUAL column names
            std::stringstream query;
            query << "INSERT INTO predictions_daily (prediction_time, target_date, symbol_id, model_id, "
                  << "predicted_open, predicted_high, predicted_low, predicted_close, "
                  << "confidence_score, model_name, created_at) VALUES ("
                  << "'" << prediction.prediction_time << "', "
                  << "'" << prediction.target_date << "', "
                  << symbol_id << ", "
                  << "1, "  // model_id default
                  << std::fixed << std::setprecision(8) 
                  << prediction.predicted_open << ", "
                  << prediction.predicted_high << ", "
                  << prediction.predicted_low << ", "
                  << prediction.predicted_close << ", "
                  << prediction.confidence_score << ", "
                  << "'Epoch Market Advisor', "
                  << "'" << get_current_timestamp() << "') "
                  << "ON CONFLICT (target_date, symbol_id, model_id) DO UPDATE SET "
                  << "predicted_open = EXCLUDED.predicted_open, "
                  << "predicted_high = EXCLUDED.predicted_high, "
                  << "predicted_low = EXCLUDED.predicted_low, "
                  << "predicted_close = EXCLUDED.predicted_close, "
                  << "confidence_score = EXCLUDED.confidence_score";
            
            bool result = db_manager.execute_query(query.str());
            
            if (result) {
                std::cout << "✅ Daily prediction saved to predictions_daily table" << std::endl;
            } else {
                std::cout << "❌ Failed to save daily prediction: " << db_manager.get_last_error() << std::endl;
            }
            
            return result;
            
        } catch (const std::exception& e) {
            std::cout << "❌ Exception saving daily prediction: " << e.what() << std::endl;
            return false;
        }
    }
    
    // Save individual prediction components to predictions_all_symbols table (ACTUAL SCHEMA)
    static bool save_prediction_components(SimpleDatabaseManager& db_manager, 
                                          const std::string& symbol,
                                          const std::string& timeframe,
                                          const std::string& prediction_type,
                                          double predicted_value,
                                          const std::string& target_time) {
        
        try {
            // Get symbol ID
            int symbol_id = db_manager.get_symbol_id(symbol);
            if (symbol_id <= 0) {
                std::cout << "❌ Could not find symbol ID for " << symbol << std::endl;
                return false;
            }
            
            std::cout << "💾 Saving " << prediction_type << " prediction: " << predicted_value << " for " << symbol << std::endl;
            
            // Insert into predictions_all_symbols table using ACTUAL column names
            std::stringstream query;
            query << "INSERT INTO predictions_all_symbols "
                  << "(prediction_time, target_time, symbol_id, model_id, timeframe, "
                  << "prediction_type, predicted_value, confidence_score, model_name, created_at) VALUES ("
                  << "'" << get_current_timestamp() << "', "
                  << "'" << target_time << "', "
                  << symbol_id << ", "
                  << "1, "  // model_id default
                  << "'" << timeframe << "', "
                  << "'" << prediction_type << "', "
                  << std::fixed << std::setprecision(8) << predicted_value << ", "
                  << "0.75, "  // Default confidence score
                  << "'Epoch Market Advisor', "
                  << "'" << get_current_timestamp() << "') "
                  << "ON CONFLICT (prediction_time, symbol_id, timeframe, prediction_type) DO UPDATE SET "
                  << "predicted_value = EXCLUDED.predicted_value, "
                  << "confidence_score = EXCLUDED.confidence_score";
            
            bool result = db_manager.execute_query(query.str());
            
            if (result) {
                std::cout << "✅ " << prediction_type << " saved to predictions_all_symbols" << std::endl;
            } else {
                std::cout << "❌ Failed to save " << prediction_type << ": " << db_manager.get_last_error() << std::endl;
            }
            
            return result;
            
        } catch (const std::exception& e) {
            std::cout << "❌ Exception saving prediction component: " << e.what() << std::endl;
            return false;
        }
    }
    
    // Save error calculation to prediction_errors_daily table (ACTUAL SCHEMA)
    static bool save_prediction_error(SimpleDatabaseManager& db_manager,
                                     const std::string& symbol,
                                     double predicted_value,
                                     double actual_value,
                                     const std::string& prediction_time) {
        
        try {
            // Get symbol ID
            int symbol_id = db_manager.get_symbol_id(symbol);
            if (symbol_id <= 0) {
                std::cout << "❌ Could not find symbol ID for " << symbol << std::endl;
                return false;
            }
            
            double absolute_error = std::abs(predicted_value - actual_value);
            double percentage_error = (absolute_error / actual_value) * 100.0;
            
            std::cout << "📊 Saving prediction error to prediction_errors_daily table:" << std::endl;
            std::cout << "   Symbol: " << symbol << std::endl;
            std::cout << "   Predicted: " << predicted_value << std::endl;
            std::cout << "   Actual:    " << actual_value << std::endl;
            std::cout << "   Error:     " << absolute_error << " (" << percentage_error << "%)" << std::endl;
            
            // Parse date from prediction_time for period_start and period_end
            std::string period_date = prediction_time.substr(0, 10); // Extract YYYY-MM-DD
            
            // Insert into prediction_errors_daily table using ACTUAL column names
            std::stringstream query;
            query << "INSERT INTO prediction_errors_daily "
                  << "(analysis_time, symbol_id, symbol_name, period_start, period_end, "
                  << "prediction_count, actual_close, predicted_close, created_at) VALUES ("
                  << "'" << get_current_timestamp() << "', "
                  << symbol_id << ", "
                  << "'" << db_manager.escape_string(symbol) << "', "
                  << "'" << period_date << "', "
                  << "'" << period_date << "', "
                  << "1, "  // prediction_count
                  << std::fixed << std::setprecision(8) << actual_value << ", "
                  << predicted_value << ", "
                  << "'" << get_current_timestamp() << "') "
                  << "ON CONFLICT (error_id) DO NOTHING";  // Simple conflict resolution
            
            bool result = db_manager.execute_query(query.str());
            
            if (result) {
                std::cout << "✅ Prediction error saved to prediction_errors_daily table" << std::endl;
            } else {
                std::cout << "❌ Failed to save prediction error: " << db_manager.get_last_error() << std::endl;
            }
            
            return result;
            
        } catch (const std::exception& e) {
            std::cout << "❌ Exception saving prediction error: " << e.what() << std::endl;
            return false;
        }
    }
};

#endif // PREDICTION_PERSISTER_H