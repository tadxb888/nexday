#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>

// Forward declarations
class SimpleDatabaseManager;
class IQFeedConnectionManager;
class Logger;

// Structure to hold historical price data for predictions
struct PriceBar {
    std::string date;
    std::string time;
    double open;
    double high;
    double low; 
    double close;
    int volume;
    
    PriceBar() : open(0), high(0), low(0), close(0), volume(0) {}
    PriceBar(const std::string& d, double o, double h, double l, double c, int v) 
        : date(d), time(""), open(o), high(h), low(l), close(c), volume(v) {}
};

// Structure to hold EMA calculation results
struct EMAResult {
    double sma10;           // SMA of first 10 bars (bootstrap)
    double final_ema;       // Final EMA value (last prediction)
    std::vector<double> ema_sequence; // Complete EMA sequence
    bool calculation_valid;
    std::string error_message;
    
    EMAResult() : sma10(0), final_ema(0), calculation_valid(false) {}
};

// Structure to hold prediction results
struct PredictionResult {
    std::string symbol;
    std::string prediction_date;
    std::string timeframe;
    
    // Daily predictions (next business day OHLC)
    double predicted_open;
    double predicted_high;
    double predicted_low;
    double predicted_close;
    
    // Intraday predictions (next interval High/Low)
    double predicted_next_high;
    double predicted_next_low;
    
    // Prediction metadata
    double base_alpha;
    double confidence_score;
    int bars_used;
    bool prediction_valid;
    std::string error_message;
    std::chrono::system_clock::time_point created_at;
    
    PredictionResult() : predicted_open(0), predicted_high(0), predicted_low(0), 
                        predicted_close(0), predicted_next_high(0), predicted_next_low(0),
                        base_alpha(0.5), confidence_score(0), bars_used(0), prediction_valid(false) {}
};

class IntegratedMarketPredictionEngine {
private:
    std::shared_ptr<SimpleDatabaseManager> db_manager_;
    std::shared_ptr<IQFeedConnectionManager> iqfeed_manager_;
    std::unique_ptr<Logger> logger_;
    
    // Model configuration - Model 1 Standard
    static constexpr double BASE_ALPHA = 0.5;  // base_alpha = 2/(P+1) where P=3
    static constexpr int MINIMUM_BARS = 15;    // Need 15 bars minimum for SMA10 + EMA
    static constexpr int BOOTSTRAP_BARS = 10;  // SMA10 for bootstrap
    
    // Helper methods for data retrieval
    bool retrieve_historical_data_from_db(const std::string& symbol, const std::string& timeframe,
                                         std::vector<PriceBar>& historical_data);
    bool fetch_fresh_data_from_iqfeed(const std::string& symbol, const std::string& timeframe,
                                     int num_bars, std::vector<PriceBar>& fresh_data);
    
    // Core EMA calculation methods
    EMAResult calculate_ema_sequence(const std::vector<PriceBar>& price_data, 
                                    const std::string& price_type = "close");
    double calculate_sma_10(const std::vector<PriceBar>& price_data, const std::string& price_type);
    
    // Prediction generation methods
    bool generate_daily_predictions(const std::string& symbol, const std::vector<PriceBar>& historical_data,
                                   PredictionResult& result);
    bool generate_intraday_predictions(const std::string& symbol, const std::string& timeframe,
                                      const std::vector<PriceBar>& historical_data,
                                      PredictionResult& result);
    
    // Database operations
    int get_or_create_symbol_id(const std::string& symbol);
    int get_or_create_model_id();
    bool save_predictions_to_db(const PredictionResult& result);
    
    // Business day calculations
    std::string get_next_business_day(const std::string& from_date);
    bool is_business_day(const std::string& date);
    
    // Data validation
    bool validate_historical_data(const std::vector<PriceBar>& data, std::string& error_message);
    bool has_sufficient_data(const std::vector<PriceBar>& data);
    
    // Error handling
    void handle_prediction_error(const std::string& operation, const std::string& error);

public:
    // Constructor and destructor
    IntegratedMarketPredictionEngine(std::shared_ptr<SimpleDatabaseManager> db_manager,
                                   std::shared_ptr<IQFeedConnectionManager> iqfeed_manager);
    ~IntegratedMarketPredictionEngine();
    
    // Main prediction methods
    bool generate_predictions_for_symbol(const std::string& symbol);
    bool generate_daily_prediction(const std::string& symbol);
    bool generate_intraday_prediction(const std::string& symbol, const std::string& timeframe);
    
    // Batch operations
    bool generate_predictions_for_all_symbols();
    bool generate_predictions_for_symbol_list(const std::vector<std::string>& symbols);
    
    // Configuration and status
    void set_minimum_bars(int bars) { /* Could make configurable if needed */ }
    void set_base_alpha(double alpha) { /* Could make configurable if needed */ }
    bool is_ready() const;
    
    // Diagnostic methods
    void print_ema_calculation_details(const std::string& symbol);
    void print_prediction_summary(const PredictionResult& result);
    void run_prediction_validation_test(const std::string& symbol);
    
    // Database integration status
    bool test_database_connection();
    bool test_iqfeed_connection();
    void print_system_status();
};