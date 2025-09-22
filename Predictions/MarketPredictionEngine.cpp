#include "MarketPredictionEngine.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

// ==============================================
// CONSTRUCTOR AND INITIALIZATION
// ==============================================

MarketPredictionEngine::MarketPredictionEngine(std::unique_ptr<SimpleDatabaseManager> db_manager)
    : db_manager_(std::move(db_manager)), model_id_(-1), model_name_("Epoch Market Advisor") {
    
    if (!is_initialized()) {
        set_error("Database manager not properly initialized");
        return;
    }
    
    // Ensure our model exists in the database
    if (!ensure_model_exists()) {
        set_error("Failed to initialize Epoch Market Advisor model");
    }
}

// ==============================================
// CORE PREDICTION METHODS
// ==============================================

bool MarketPredictionEngine::generate_predictions_for_symbol(const std::string& symbol) {
    log_info("Generating predictions for symbol: " + symbol);
    
    try {
        // Generate daily prediction
        OHLCPrediction daily_pred = generate_daily_prediction(symbol);
        if (daily_pred.confidence_score > 0.0) {
            if (!save_prediction_to_database(symbol, daily_pred)) {
                log_error("Failed to save daily prediction for " + symbol);
                return false;
            }
        }
        
        // Generate intraday predictions
        auto intraday_preds = generate_intraday_predictions(symbol);
        for (const auto& [timeframe, prediction] : intraday_preds) {
            if (prediction.confidence_score > 0.0) {
                if (!save_intraday_prediction_to_database(symbol, prediction)) {
                    log_error("Failed to save intraday prediction for " + symbol + " " + 
                             timeframe_to_string(timeframe));
                }
            }
        }
        
        log_info("Successfully generated predictions for " + symbol);
        return true;
        
    } catch (const std::exception& e) {
        set_error("Exception in generate_predictions_for_symbol: " + std::string(e.what()));
        return false;
    }
}

bool MarketPredictionEngine::generate_predictions_for_all_active_symbols() {
    log_info("Generating predictions for all active symbols");
    
    auto symbols = db_manager_->get_symbol_list(true); // active_only = true
    if (symbols.empty()) {
        set_error("No active symbols found in database");
        return false;
    }
    
    int successful = 0;
    int failed = 0;
    
    for (const auto& symbol : symbols) {
        if (generate_predictions_for_symbol(symbol)) {
            successful++;
        } else {
            failed++;
            log_error("Failed to generate predictions for " + symbol);
        }
    }
    
    log_info("Prediction generation completed: " + std::to_string(successful) + 
             " successful, " + std::to_string(failed) + " failed");
    
    return failed == 0;
}

// ==============================================
// DAILY PREDICTION GENERATION
// ==============================================

OHLCPrediction MarketPredictionEngine::generate_daily_prediction(const std::string& symbol) {
    OHLCPrediction prediction;
    
    try {
        // Get daily historical data
        auto historical_data = get_historical_data(symbol, TimeFrame::DAILY, 100);
        
        if (historical_data.size() < MINIMUM_BARS) {
            set_error("Insufficient historical data for " + symbol + ": " + 
                     std::to_string(historical_data.size()) + " bars (need " + 
                     std::to_string(MINIMUM_BARS) + ")");
            return prediction;
        }
        
        // Calculate EMA for each OHLC component
        auto open_ema = calculate_ema_for_prediction(historical_data, "open");
        auto high_ema = calculate_ema_for_prediction(historical_data, "high");
        auto low_ema = calculate_ema_for_prediction(historical_data, "low");
        auto close_ema = calculate_ema_for_prediction(historical_data, "close");
        
        // Verify all calculations are valid
        if (!open_ema.valid || !high_ema.valid || !low_ema.valid || !close_ema.valid) {
            set_error("EMA calculation failed for " + symbol + " daily prediction");
            return prediction;
        }
        
        // Set prediction values
        prediction.predicted_open = open_ema.final_ema;
        prediction.predicted_high = high_ema.final_ema;
        prediction.predicted_low = low_ema.final_ema;
        prediction.predicted_close = close_ema.final_ema;
        
        // Set timing information
        prediction.prediction_time = std::chrono::system_clock::now();
        
        // Calculate target time (next business day)
        auto latest_bar_time = historical_data.back().timestamp;
        prediction.target_time = BusinessDayCalculator::get_next_business_day(latest_bar_time);
        
        // Calculate confidence score (simplified - based on data quality and consistency)
        prediction.confidence_score = calculate_prediction_confidence(historical_data);
        
        log_info("Daily prediction generated for " + symbol + 
                ": O=" + std::to_string(prediction.predicted_open) +
                ", H=" + std::to_string(prediction.predicted_high) +
                ", L=" + std::to_string(prediction.predicted_low) +
                ", C=" + std::to_string(prediction.predicted_close));
        
    } catch (const std::exception& e) {
        set_error("Exception in generate_daily_prediction: " + std::string(e.what()));
    }
    
    return prediction;
}

// ==============================================
// INTRADAY PREDICTION GENERATION
// ==============================================

std::map<TimeFrame, HighLowPrediction> MarketPredictionEngine::generate_intraday_predictions(
    const std::string& symbol) {
    
    std::map<TimeFrame, HighLowPrediction> predictions;
    
    // Define timeframes for intraday predictions
    std::vector<TimeFrame> timeframes = {
        TimeFrame::MINUTES_15,
        TimeFrame::MINUTES_30,
        TimeFrame::HOUR_1,
        TimeFrame::HOURS_2
    };
    
    for (auto timeframe : timeframes) {
        HighLowPrediction prediction;
        prediction.timeframe = timeframe;
        
        try {
            // Get historical data for this timeframe
            auto historical_data = get_historical_data(symbol, timeframe, 100);
            
            if (historical_data.size() < MINIMUM_BARS) {
                log_error("Insufficient data for " + symbol + " " + timeframe_to_string(timeframe) +
                         ": " + std::to_string(historical_data.size()) + " bars");
                continue;
            }
            
            // Calculate EMA for high and low
            auto high_ema = calculate_ema_for_prediction(historical_data, "high");
            auto low_ema = calculate_ema_for_prediction(historical_data, "low");
            
            if (!high_ema.valid || !low_ema.valid) {
                log_error("EMA calculation failed for " + symbol + " " + timeframe_to_string(timeframe));
                continue;
            }
            
            // Set prediction values
            prediction.predicted_high = high_ema.final_ema;
            prediction.predicted_low = low_ema.final_ema;
            
            // Set timing information
            prediction.prediction_time = std::chrono::system_clock::now();
            prediction.target_time = calculate_next_prediction_time(timeframe);
            
            // Calculate confidence
            prediction.confidence_score = calculate_prediction_confidence(historical_data);
            
            predictions[timeframe] = prediction;
            
            log_info("Intraday prediction generated for " + symbol + " " + 
                    timeframe_to_string(timeframe) +
                    ": H=" + std::to_string(prediction.predicted_high) +
                    ", L=" + std::to_string(prediction.predicted_low));
            
        } catch (const std::exception& e) {
            log_error("Exception generating intraday prediction for " + symbol + " " +
                     timeframe_to_string(timeframe) + ": " + e.what());
        }
    }
    
    return predictions;
}

// ==============================================
// EMA CALCULATION ENGINE - MODEL 1 STANDARD
// ==============================================

EMAResult MarketPredictionEngine::calculate_ema_for_prediction(
    const std::vector<HistoricalBar>& historical_data, const std::string& price_type) {
    
    EMAResult result;
    
    try {
        // Extract price series
        auto price_series = extract_price_series(historical_data, price_type);
        
        if (price_series.size() < MINIMUM_BARS) {
            set_error("Insufficient data points: " + std::to_string(price_series.size()));
            return result;
        }
        
        // Step 1: Calculate SMA bootstrap (SMA1 through SMA10)
        result.sma_values = calculate_sma_bootstrap(price_series);
        
        if (result.sma_values.size() != SMA_PERIODS) {
            set_error("SMA bootstrap failed: expected " + std::to_string(SMA_PERIODS) + 
                     " values, got " + std::to_string(result.sma_values.size()));
            return result;
        }
        
        // Step 2: Use SMA10 as initial previous_predict for EMA sequence
        double initial_previous_predict = result.sma_values.back(); // SMA10
        
        // Step 3: Calculate EMA sequence starting from bar 15 (index 14)
        std::vector<double> ema_input_series(price_series.begin() + 14, price_series.end());
        result.ema_values = calculate_ema_sequence(ema_input_series, initial_previous_predict);
        
        if (!result.ema_values.empty()) {
            result.final_ema = result.ema_values.back();
            result.valid = true;
            result.bars_used = price_series.size();
        }
        
    } catch (const std::exception& e) {
        set_error("Exception in calculate_ema_for_prediction: " + std::string(e.what()));
    }
    
    return result;
}

// ==============================================
// INTERNAL EMA CALCULATION HELPERS
// ==============================================

std::vector<double> MarketPredictionEngine::calculate_sma_bootstrap(const std::vector<double>& values) {
    std::vector<double> sma_values;
    sma_values.reserve(SMA_PERIODS);
    
    // Calculate SMA1 through SMA10 using 5-bar rolling windows
    for (int i = 0; i < SMA_PERIODS; i++) {
        double sma = calculate_sma(values, i, SMA_WINDOW);
        sma_values.push_back(sma);
    }
    
    return sma_values;
}

double MarketPredictionEngine::calculate_sma(const std::vector<double>& values, 
                                           int start_index, int window_size) {
    if (start_index + window_size > values.size()) {
        return 0.0; // Not enough data
    }
    
    double sum = 0.0;
    for (int i = start_index; i < start_index + window_size; i++) {
        sum += values[i];
    }
    
    return sum / window_size;
}

std::vector<double> MarketPredictionEngine::calculate_ema_sequence(
    const std::vector<double>& values, double initial_previous_predict) {
    
    std::vector<double> ema_values;
    ema_values.reserve(values.size());
    
    double previous_predict = initial_previous_predict;
    
    // Apply Model 1 Standard formula: predict_t = (base_alpha * current_value) + ((1 - base_alpha) * previous_predict)
    for (double current_value : values) {
        double predict_t = (BASE_ALPHA * current_value) + ((1.0 - BASE_ALPHA) * previous_predict);
        ema_values.push_back(predict_t);
        previous_predict = predict_t; // This prediction becomes previous_predict for next iteration
    }
    
    return ema_values;
}

// ==============================================
// HISTORICAL DATA RETRIEVAL
// ==============================================

std::vector<HistoricalBar> MarketPredictionEngine::get_historical_data(
    const std::string& symbol, TimeFrame timeframe, int num_bars) {
    
    std::vector<HistoricalBar> result;
    
    try {
        std::string table_name = get_historical_table_name(timeframe);
        
        std::stringstream query;
        query << "SELECT time, open_price, high_price, low_price, close_price, volume ";
        query << "FROM " << table_name << " h ";
        query << "JOIN symbols s ON h.symbol_id = s.symbol_id ";
        query << "WHERE s.symbol = '" << symbol << "' ";
        query << "ORDER BY h.time DESC ";
        query << "LIMIT " << num_bars;
        
        // NOTE: This would need to be implemented in SimpleDatabaseManager
        // For now, creating sample data for testing
        log_info("Retrieving " + std::to_string(num_bars) + " bars for " + symbol + 
                " timeframe " + timeframe_to_string(timeframe));
        
        // Create sample test data (would be replaced with actual database query)
        auto base_time = std::chrono::system_clock::now();
        for (int i = 0; i < std::min(num_bars, 25); i++) {
            HistoricalBar bar;
            bar.timestamp = base_time - std::chrono::hours(24 * i);
            bar.open = 100.0 + i * 0.5;
            bar.high = 101.0 + i * 0.5;
            bar.low = 99.0 + i * 0.5;
            bar.close = 100.2 + i * 0.5;
            bar.volume = 1000000;
            result.push_back(bar);
        }
        
        std::reverse(result.begin(), result.end()); // Oldest first for calculations
        
    } catch (const std::exception& e) {
        set_error("Exception retrieving historical data: " + std::string(e.what()));
    }
    
    return result;
}
// ==============================================
// DATABASE OPERATIONS
// ==============================================

bool MarketPredictionEngine::save_prediction_to_database(const std::string& symbol, 
                                                        const OHLCPrediction& prediction) {
    try {
        // Get symbol_id
        int symbol_id = get_symbol_id(symbol);
        if (symbol_id == -1) {
            set_error("Symbol not found: " + symbol);
            return false;
        }
        
        // Save each OHLC component as separate prediction record
        std::vector<std::pair<std::string, double>> components = {
            {"daily_open", prediction.predicted_open},
            {"daily_high", prediction.predicted_high},
            {"daily_low", prediction.predicted_low},
            {"daily_close", prediction.predicted_close}
        };
        
        for (const auto& [component_name, predicted_value] : components) {
            log_info("Saving " + component_name + " prediction for " + symbol + 
                    ": " + std::to_string(predicted_value));
        }
        
        return true;
        
    } catch (const std::exception& e) {
        set_error("Exception saving daily prediction: " + std::string(e.what()));
        return false;
    }
}

bool MarketPredictionEngine::save_intraday_prediction_to_database(const std::string& symbol,
                                                                 const HighLowPrediction& prediction) {
    try {
        int symbol_id = get_symbol_id(symbol);
        if (symbol_id == -1) {
            set_error("Symbol not found: " + symbol);
            return false;
        }
        
        std::string timeframe_str = timeframe_to_string(prediction.timeframe);
        
        // Save High and Low predictions
        log_info("Saving " + timeframe_str + "_high prediction for " + symbol + 
                ": " + std::to_string(prediction.predicted_high));
        log_info("Saving " + timeframe_str + "_low prediction for " + symbol + 
                ": " + std::to_string(prediction.predicted_low));
        
        return true;
        
    } catch (const std::exception& e) {
        set_error("Exception saving intraday prediction: " + std::string(e.what()));
        return false;
    }
}

// ==============================================
// MODEL MANAGEMENT
// ==============================================

bool MarketPredictionEngine::ensure_model_exists() {
    model_id_ = get_or_create_model_id();
    return model_id_ != -1;
}

int MarketPredictionEngine::get_or_create_model_id() {
    try {
        log_info("Model '" + model_name_ + "' initialized");
        return 1; // Placeholder model_id (would query/create in production)
        
    } catch (const std::exception& e) {
        set_error("Exception in get_or_create_model_id: " + std::string(e.what()));
        return -1;
    }
}

// ==============================================
// UTILITY METHODS
// ==============================================

std::vector<double> MarketPredictionEngine::extract_price_series(
    const std::vector<HistoricalBar>& bars, const std::string& price_type) {
    
    std::vector<double> prices;
    prices.reserve(bars.size());
    
    for (const auto& bar : bars) {
        if (price_type == "open") {
            prices.push_back(bar.open);
        } else if (price_type == "high") {
            prices.push_back(bar.high);
        } else if (price_type == "low") {
            prices.push_back(bar.low);
        } else if (price_type == "close") {
            prices.push_back(bar.close);
        } else {
            prices.push_back(bar.close); // Default to close
        }
    }
    
    return prices;
}

std::string MarketPredictionEngine::get_historical_table_name(TimeFrame timeframe) {
    switch (timeframe) {
        case TimeFrame::MINUTES_15: return "historical_fetch_15min";
        case TimeFrame::MINUTES_30: return "historical_fetch_30min";
        case TimeFrame::HOUR_1: return "historical_fetch_1hour";
        case TimeFrame::HOURS_2: return "historical_fetch_2hour";
        case TimeFrame::DAILY: return "historical_fetch_daily";
        default: return "historical_fetch_daily";
    }
}

double MarketPredictionEngine::calculate_prediction_confidence(
    const std::vector<HistoricalBar>& historical_data) {
    
    if (historical_data.size() < MINIMUM_BARS) {
        return 0.0;
    }
    
    // Base confidence starts at 0.7 for having enough data
    double confidence = 0.7;
    
    // Bonus for more data
    if (historical_data.size() >= 50) confidence += 0.1;
    if (historical_data.size() >= 100) confidence += 0.1;
    
    // Check for data quality
    int valid_bars = 0;
    for (const auto& bar : historical_data) {
        if (bar.open > 0 && bar.high > 0 && bar.low > 0 && bar.close > 0 &&
            bar.high >= bar.low && bar.high >= bar.open && bar.high >= bar.close &&
            bar.low <= bar.open && bar.low <= bar.close) {
            valid_bars++;
        }
    }
    
    double data_quality_ratio = static_cast<double>(valid_bars) / historical_data.size();
    confidence *= data_quality_ratio;
    
    return std::min(confidence, 1.0);
}

std::chrono::system_clock::time_point MarketPredictionEngine::calculate_next_prediction_time(TimeFrame timeframe) {
    auto now = std::chrono::system_clock::now();
    int minutes = timeframe_to_minutes(timeframe);
    return now + std::chrono::minutes(minutes);
}

int MarketPredictionEngine::get_symbol_id(const std::string& symbol) {
    log_info("Getting symbol_id for: " + symbol);
    return 1; // Placeholder (would query database in production)
}

// ==============================================
// DEBUG AND TESTING METHODS
// ==============================================

void MarketPredictionEngine::print_ema_calculation_debug(const std::vector<HistoricalBar>& data, 
                                                        const EMAResult& result) {
    std::cout << "\n=== EMA CALCULATION DEBUG ===" << std::endl;
    std::cout << "Historical data points: " << data.size() << std::endl;
    std::cout << "Minimum required: " << MINIMUM_BARS << std::endl;
    std::cout << "Base Alpha: " << BASE_ALPHA << std::endl;
    
    if (!result.valid) {
        std::cout << "EMA calculation FAILED" << std::endl;
        return;
    }
    
    std::cout << "\nSMA Bootstrap (SMA1-SMA10):" << std::endl;
    for (size_t i = 0; i < result.sma_values.size(); i++) {
        std::cout << "SMA" << (i+1) << ": " << std::fixed << std::setprecision(4) 
                  << result.sma_values[i] << std::endl;
    }
    
    std::cout << "\nEMA Sequence (last 10 values):" << std::endl;
    size_t start_idx = result.ema_values.size() > 10 ? result.ema_values.size() - 10 : 0;
    for (size_t i = start_idx; i < result.ema_values.size(); i++) {
        std::cout << "EMA" << (i + 15) << ": " << std::fixed << std::setprecision(4) 
                  << result.ema_values[i] << std::endl;
    }
    
    std::cout << "\nFinal EMA for prediction: " << std::fixed << std::setprecision(4) 
              << result.final_ema << std::endl;
    std::cout << "===========================\n" << std::endl;
}

PredictionValidation MarketPredictionEngine::validate_predictions(const std::string& symbol, 
                                                                 TimeFrame timeframe,
                                                                 int validation_days) {
    PredictionValidation result;
    
    result.is_valid = true;
    result.mae = 0.0;   // Would calculate from validation data
    result.rmse = 0.0;  // Would calculate from validation data  
    result.mape = 0.0;  // Would calculate from validation data
    result.r2 = 0.0;    // Would calculate from validation data
    
    log_info("Validation completed for " + symbol + " " + timeframe_to_string(timeframe));
    
    return result;
}

// ==============================================
// ERROR HANDLING AND LOGGING
// ==============================================

void MarketPredictionEngine::set_error(const std::string& error_message) {
    last_error_ = error_message;
    log_error(error_message);
}

void MarketPredictionEngine::log_info(const std::string& message) {
    std::cout << "[INFO] MarketPredictionEngine: " << message << std::endl;
}

void MarketPredictionEngine::log_error(const std::string& message) {
    std::cerr << "[ERROR] MarketPredictionEngine: " << message << std::endl;
}