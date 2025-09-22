#pragma once

#include "PredictionTypes.h"
#include "BusinessDayCalculator.h"
#include "database_simple.h"
#include <memory>
#include <vector>
#include <map>
#include <string>
#include <chrono>

// ==============================================
// MARKET PREDICTION ENGINE - EPOCH MARKET ADVISOR
// ==============================================

class MarketPredictionEngine {
private:
    std::unique_ptr<SimpleDatabaseManager> db_manager_;
    int model_id_;
    std::string model_name_;
    std::string last_error_;
    
    // Model parameters
    static constexpr double BASE_ALPHA = Model1Parameters::BASE_ALPHA;
    static constexpr int MINIMUM_BARS = Model1Parameters::MINIMUM_BARS;
    static constexpr int SMA_PERIODS = Model1Parameters::SMA_PERIODS;
    static constexpr int SMA_WINDOW = Model1Parameters::SMA_WINDOW;

public:
    // Constructor
    explicit MarketPredictionEngine(std::unique_ptr<SimpleDatabaseManager> db_manager);
    ~MarketPredictionEngine() = default;
    
    // Core prediction methods
    bool generate_predictions_for_symbol(const std::string& symbol);
    bool generate_predictions_for_all_active_symbols();
    
    // Specific prediction types
    OHLCPrediction generate_daily_prediction(const std::string& symbol);
    std::map<TimeFrame, HighLowPrediction> generate_intraday_predictions(const std::string& symbol);
    
    // EMA calculation engine
    EMAResult calculate_ema_for_prediction(const std::vector<HistoricalBar>& historical_data,
                                          const std::string& price_type = "close");
    
    // Historical data retrieval
    std::vector<HistoricalBar> get_historical_data(const std::string& symbol, 
                                                  TimeFrame timeframe, 
                                                  int num_bars = 100);
    
    // Database operations
    bool save_prediction_to_database(const std::string& symbol, const OHLCPrediction& prediction);
    bool save_intraday_prediction_to_database(const std::string& symbol, 
                                             const HighLowPrediction& prediction);
    
    // Model management
    bool ensure_model_exists();
    int get_or_create_model_id();
    
    // Validation and testing
    PredictionValidation validate_predictions(const std::string& symbol, 
                                            TimeFrame timeframe,
                                            int validation_days = 30);
    
    // Utility methods
    std::string get_last_error() const { return last_error_; }
    bool is_initialized() const { return db_manager_ && db_manager_->is_connected(); }
    
    // Debug and logging
    void print_ema_calculation_debug(const std::vector<HistoricalBar>& data, 
                                    const EMAResult& result);
    void print_prediction_summary(const SymbolPrediction& prediction);

private:
    // Internal EMA calculation helpers
    double calculate_sma(const std::vector<double>& values, int start_index, int window_size);
    std::vector<double> calculate_sma_bootstrap(const std::vector<double>& values);
    std::vector<double> calculate_ema_sequence(const std::vector<double>& values, 
                                              double initial_previous_predict);
    
    // Historical data processing
    std::vector<double> extract_price_series(const std::vector<HistoricalBar>& bars, 
                                            const std::string& price_type);
    bool validate_historical_data(const std::vector<HistoricalBar>& data);
    
    // Database table name helpers
    std::string get_historical_table_name(TimeFrame timeframe);
    std::string get_prediction_component_name(const std::string& base_name, TimeFrame timeframe);
    
    // Time and business day calculations
    std::chrono::system_clock::time_point calculate_next_prediction_time(TimeFrame timeframe);
    std::chrono::system_clock::time_point get_latest_complete_bar_time(TimeFrame timeframe);
    
    // Additional helper methods
    int get_symbol_id(const std::string& symbol);
    double calculate_prediction_confidence(const std::vector<HistoricalBar>& historical_data);
    
    // Error handling
    void set_error(const std::string& error_message);
    void log_info(const std::string& message);
    void log_error(const std::string& message);
};