#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <map>

// ==============================================
// PREDICTION DATA STRUCTURES
// ==============================================

enum class TimeFrame {
    MINUTES_15 = 15,
    MINUTES_30 = 30,
    HOUR_1 = 60,
    HOURS_2 = 120,
    DAILY = 1440
};

enum class PredictionType {
    OHLC_DAILY,      // Next business day OHLC predictions
    HIGH_LOW_INTRADAY // Next interval High/Low predictions
};

// Historical bar data structure
struct HistoricalBar {
    std::chrono::system_clock::time_point timestamp;
    double open;
    double high;
    double low;
    double close;
    long long volume;
    
    HistoricalBar() : open(0.0), high(0.0), low(0.0), close(0.0), volume(0) {}
    
    HistoricalBar(std::chrono::system_clock::time_point ts, double o, double h, double l, double c, long long v)
        : timestamp(ts), open(o), high(h), low(l), close(c), volume(v) {}
};

// Single OHLC prediction
struct OHLCPrediction {
    double predicted_open;
    double predicted_high;
    double predicted_low;
    double predicted_close;
    double confidence_score;
    std::chrono::system_clock::time_point prediction_time;
    std::chrono::system_clock::time_point target_time;
    
    OHLCPrediction() : predicted_open(0.0), predicted_high(0.0), predicted_low(0.0), 
                       predicted_close(0.0), confidence_score(0.0) {}
};

// High/Low prediction for intraday intervals
struct HighLowPrediction {
    double predicted_high;
    double predicted_low;
    double confidence_score;
    TimeFrame timeframe;
    std::chrono::system_clock::time_point prediction_time;
    std::chrono::system_clock::time_point target_time;
    
    HighLowPrediction() : predicted_high(0.0), predicted_low(0.0), confidence_score(0.0), 
                          timeframe(TimeFrame::MINUTES_15) {}
};

// Complete prediction set for a symbol
struct SymbolPrediction {
    std::string symbol;
    int symbol_id;
    
    // Daily predictions
    OHLCPrediction daily_prediction;
    
    // Intraday predictions by timeframe
    std::map<TimeFrame, HighLowPrediction> intraday_predictions;
    
    // Model information
    int model_id;
    std::string model_name;
    
    SymbolPrediction() : symbol_id(-1), model_id(-1) {}
};

// EMA calculation result
struct EMAResult {
    std::vector<double> sma_values;    // SMA1 through SMA10
    std::vector<double> ema_values;    // EMA11 through final EMA
    double final_ema;                  // The final EMA value for next prediction
    bool valid;                        // Whether calculation was successful
    int bars_used;                     // Number of bars used in calculation
    
    EMAResult() : final_ema(0.0), valid(false), bars_used(0) {}
};

// Model 1 Standard parameters
struct Model1Parameters {
    static constexpr double BASE_ALPHA = 0.5;  // base_alpha = 2/(P+1) where P=3
    static constexpr int MINIMUM_BARS = 15;    // Minimum bars needed for prediction
    static constexpr int SMA_PERIODS = 10;     // Number of SMA calculations needed
    static constexpr int SMA_WINDOW = 5;       // SMA rolling window size
};

// Prediction validation result
struct PredictionValidation {
    bool is_valid;
    std::string error_message;
    double mae;  // Mean Absolute Error
    double rmse; // Root Mean Square Error  
    double mape; // Mean Absolute Percentage Error
    double r2;   // R-squared coefficient
    
    PredictionValidation() : is_valid(false), mae(0.0), rmse(0.0), mape(0.0), r2(0.0) {}
};

// Utility functions for timeframe conversion
inline std::string timeframe_to_string(TimeFrame tf) {
    switch (tf) {
        case TimeFrame::MINUTES_15: return "15min";
        case TimeFrame::MINUTES_30: return "30min";
        case TimeFrame::HOUR_1: return "1hour";
        case TimeFrame::HOURS_2: return "2hour";
        case TimeFrame::DAILY: return "daily";
        default: return "unknown";
    }
}

inline TimeFrame string_to_timeframe(const std::string& str) {
    if (str == "15min") return TimeFrame::MINUTES_15;
    if (str == "30min") return TimeFrame::MINUTES_30;
    if (str == "1hour") return TimeFrame::HOUR_1;
    if (str == "2hour") return TimeFrame::HOURS_2;
    if (str == "daily") return TimeFrame::DAILY;
    return TimeFrame::DAILY; // Default
}

inline int timeframe_to_minutes(TimeFrame tf) {
    return static_cast<int>(tf);
}