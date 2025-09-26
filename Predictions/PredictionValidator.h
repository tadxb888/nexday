#pragma once

#include "database_simple.h"
#include "PredictionTypes.h"
#include <memory>
#include <vector>
#include <string>
#include <chrono>
#include <map>

// ==============================================
// PREDICTION VALIDATION AND ERROR CALCULATION SYSTEM
// ==============================================

struct ValidationResult {
    bool success = false;
    int predictions_validated = 0;
    int predictions_found = 0;
    std::string error_message;
    
    // Error metrics calculated
    double mae = 0.0;
    double rmse = 0.0;
    double mape = 0.0;
    double smape = 0.0;
    double r_squared = 0.0;
    double matthews_correlation = 0.0;
    double directional_accuracy = 0.0;
    double max_deviation = 0.0;
    double avg_deviation = 0.0;
};

struct ErrorMetrics {
    double mae = 0.0;           // Mean Absolute Error
    double rmse = 0.0;          // Root Mean Square Error  
    double mape = 0.0;          // Mean Absolute Percentage Error
    double smape = 0.0;         // Symmetric Mean Absolute Percentage Error
    double r_squared = 0.0;     // R-squared coefficient
    double matthews_correlation = 0.0; // Matthews Correlation Coefficient
    double directional_accuracy = 0.0; // Percentage of correct direction predictions
    double max_deviation = 0.0; // Maximum absolute deviation
    double avg_deviation = 0.0; // Average absolute deviation
    int sample_count = 0;       // Number of samples used in calculation
};

class PredictionValidator {
private:
    std::unique_ptr<SimpleDatabaseManager> db_manager_;
    std::string last_error_;

public:
    // Constructor
    explicit PredictionValidator(std::unique_ptr<SimpleDatabaseManager> db_manager);
    ~PredictionValidator() = default;

    // Core validation methods
    ValidationResult validate_daily_predictions(const std::string& symbol = "", int days_back = 30);
    ValidationResult validate_intraday_predictions(TimeFrame timeframe, const std::string& symbol = "", int days_back = 7);
    ValidationResult validate_all_predictions(int days_back = 30);

    // Validation for specific date ranges
    ValidationResult validate_predictions_for_date_range(const std::string& start_date, const std::string& end_date);
    ValidationResult validate_predictions_for_symbol_and_date(const std::string& symbol, const std::string& target_date);

    // Error calculation methods
    ErrorMetrics calculate_error_metrics(const std::vector<double>& actual_values, 
                                        const std::vector<double>& predicted_values);
    ErrorMetrics calculate_directional_accuracy(const std::vector<double>& actual_values,
                                               const std::vector<double>& predicted_values,
                                               const std::vector<double>& previous_values);

    // Database update methods
    bool update_prediction_validation(const std::string& symbol, TimeFrame timeframe,
                                     const std::string& prediction_type, const std::string& target_time,
                                     double actual_value, double predicted_value);
    bool save_error_analysis(const std::string& symbol, TimeFrame timeframe, const ErrorMetrics& metrics,
                            const std::string& period_start, const std::string& period_end);

    // Historical data matching
    std::vector<std::pair<double, double>> match_predictions_with_actuals(
        const std::string& symbol, TimeFrame timeframe, const std::string& prediction_type,
        const std::string& start_date, const std::string& end_date);

    // Utility methods
    std::string get_last_error() const { return last_error_; }
    bool is_initialized() const { return db_manager_ && db_manager_->is_connected(); }

    // Report generation
    void generate_validation_report(const std::string& symbol = "", int days_back = 30);
    void generate_error_summary_report();

private:
    // Internal validation helpers
    bool validate_daily_prediction_for_date(const std::string& symbol, const std::string& target_date);
    bool validate_intraday_prediction_for_period(const std::string& symbol, TimeFrame timeframe, 
                                                 const std::string& target_time);

    // Historical data retrieval for validation
    std::map<std::string, double> get_actual_daily_ohlc(const std::string& symbol, const std::string& date);
    std::map<std::string, double> get_actual_intraday_hl(const std::string& symbol, TimeFrame timeframe, 
                                                         const std::string& target_time);

    // Error calculation helpers
    double calculate_mae(const std::vector<double>& actual, const std::vector<double>& predicted);
    double calculate_rmse(const std::vector<double>& actual, const std::vector<double>& predicted);
    double calculate_mape(const std::vector<double>& actual, const std::vector<double>& predicted);
    double calculate_smape(const std::vector<double>& actual, const std::vector<double>& predicted);
    double calculate_r_squared(const std::vector<double>& actual, const std::vector<double>& predicted);
    double calculate_matthews_correlation(const std::vector<double>& actual, const std::vector<double>& predicted,
                                         const std::vector<double>& previous);

    // Database helpers
    int get_symbol_id(const std::string& symbol);
    std::string get_error_table_name(TimeFrame timeframe);
    std::string format_timestamp(const std::chrono::system_clock::time_point& tp);
    std::chrono::system_clock::time_point parse_date_string(const std::string& date_str);

    // Logging
    void log_info(const std::string& message);
    void log_error(const std::string& message);
    void set_error(const std::string& error_message);
};

// ==============================================
// IMPLEMENTATION
// ==============================================

PredictionValidator::PredictionValidator(std::unique_ptr<SimpleDatabaseManager> db_manager)
    : db_manager_(std::move(db_manager)) {
    
    if (!is_initialized()) {
        set_error("Database manager not properly initialized");
    }
}

// ==============================================
// CORE VALIDATION METHODS
// ==============================================

ValidationResult PredictionValidator::validate_daily_predictions(const std::string& symbol, int days_back) {
    ValidationResult result;
    
    try {
        log_info("Starting daily prediction validation for symbol: " + 
                (symbol.empty() ? "ALL" : symbol) + ", days back: " + std::to_string(days_back));

        // Build query to get unvalidated daily predictions
        std::stringstream query;
        query << "SELECT pd.prediction_id, s.symbol, pd.target_date, ";
        query << "pd.predicted_open, pd.predicted_high, pd.predicted_low, pd.predicted_close, ";
        query << "pd.symbol_id FROM predictions_daily pd ";
        query << "JOIN symbols s ON pd.symbol_id = s.symbol_id ";
        query << "WHERE pd.is_validated = FALSE ";
        query << "AND pd.target_date >= CURRENT_DATE - INTERVAL '" << days_back << " days' ";
        query << "AND pd.target_date <= CURRENT_DATE - INTERVAL '1 day' "; // Only validate completed days
        
        if (!symbol.empty()) {
            query << "AND s.symbol = '" << symbol << "' ";
        }
        
        query << "ORDER BY pd.target_date DESC";

        PGresult* predictions = db_manager_->execute_query_with_result(query.str());
        if (!predictions) {
            result.error_message = "Failed to retrieve daily predictions for validation";
            return result;
        }

        int prediction_count = PQntuples(predictions);
        result.predictions_found = prediction_count;
        
        log_info("Found " + std::to_string(prediction_count) + " daily predictions to validate");

        // Validate each prediction
        for (int i = 0; i < prediction_count; i++) {
            std::string pred_symbol = PQgetvalue(predictions, i, 1);
            std::string target_date = PQgetvalue(predictions, i, 2);
            
            if (validate_daily_prediction_for_date(pred_symbol, target_date)) {
                result.predictions_validated++;
            }
        }

        PQclear(predictions);

        // Calculate overall error metrics if we validated any predictions
        if (result.predictions_validated > 0) {
            // Get all validated predictions for error calculation
            auto matched_data = match_predictions_with_actuals(symbol, TimeFrame::DAILY, "close", 
                                                              "", ""); // All data
            
            if (matched_data.size() > 0) {
                std::vector<double> actual_values, predicted_values;
                for (const auto& [actual, predicted] : matched_data) {
                    actual_values.push_back(actual);
                    predicted_values.push_back(predicted);
                }
                
                ErrorMetrics metrics = calculate_error_metrics(actual_values, predicted_values);
                result.mae = metrics.mae;
                result.rmse = metrics.rmse;
                result.mape = metrics.mape;
                result.smape = metrics.smape;
                result.r_squared = metrics.r_squared;
                
                // Save error analysis to database
                std::string period_start = std::to_string(days_back) + " days ago";
                std::string period_end = "today";
                save_error_analysis(symbol, TimeFrame::DAILY, metrics, period_start, period_end);
            }
        }

        result.success = true;
        log_info("Daily validation completed: " + std::to_string(result.predictions_validated) + 
                " / " + std::to_string(result.predictions_found) + " predictions validated");

    } catch (const std::exception& e) {
        result.error_message = "Exception in validate_daily_predictions: " + std::string(e.what());
        log_error(result.error_message);
    }

    return result;
}

ValidationResult PredictionValidator::validate_intraday_predictions(TimeFrame timeframe, 
                                                                   const std::string& symbol, int days_back) {
    ValidationResult result;
    
    try {
        std::string timeframe_str = timeframe_to_string(timeframe);
        log_info("Starting intraday prediction validation for " + timeframe_str + 
                ", symbol: " + (symbol.empty() ? "ALL" : symbol));

        // Build query to get unvalidated intraday predictions
        std::stringstream query;
        query << "SELECT pas.prediction_id, s.symbol, pas.target_time, pas.prediction_type, ";
        query << "pas.predicted_value, pas.symbol_id FROM predictions_all_symbols pas ";
        query << "JOIN symbols s ON pas.symbol_id = s.symbol_id ";
        query << "WHERE pas.is_validated = FALSE ";
        query << "AND pas.timeframe = '" << timeframe_str << "' ";
        query << "AND pas.target_time >= CURRENT_TIMESTAMP - INTERVAL '" << days_back << " days' ";
        query << "AND pas.target_time <= CURRENT_TIMESTAMP - INTERVAL '1 hour' "; // Only validate completed periods
        
        if (!symbol.empty()) {
            query << "AND s.symbol = '" << symbol << "' ";
        }
        
        query << "ORDER BY pas.target_time DESC";

        PGresult* predictions = db_manager_->execute_query_with_result(query.str());
        if (!predictions) {
            result.error_message = "Failed to retrieve intraday predictions for validation";
            return result;
        }

        int prediction_count = PQntuples(predictions);
        result.predictions_found = prediction_count;
        
        log_info("Found " + std::to_string(prediction_count) + " intraday predictions to validate");

        // Validate each prediction
        for (int i = 0; i < prediction_count; i++) {
            std::string pred_symbol = PQgetvalue(predictions, i, 1);
            std::string target_time = PQgetvalue(predictions, i, 2);
            
            if (validate_intraday_prediction_for_period(pred_symbol, timeframe, target_time)) {
                result.predictions_validated++;
            }
        }

        PQclear(predictions);
        result.success = true;
        
        log_info("Intraday validation completed: " + std::to_string(result.predictions_validated) + 
                " / " + std::to_string(result.predictions_found) + " predictions validated");

    } catch (const std::exception& e) {
        result.error_message = "Exception in validate_intraday_predictions: " + std::string(e.what());
        log_error(result.error_message);
    }

    return result;
}

ValidationResult PredictionValidator::validate_all_predictions(int days_back) {
    ValidationResult combined_result;
    
    try {
        log_info("Starting comprehensive prediction validation for last " + std::to_string(days_back) + " days");

        // Validate daily predictions
        ValidationResult daily_result = validate_daily_predictions("", days_back);
        combined_result.predictions_found += daily_result.predictions_found;
        combined_result.predictions_validated += daily_result.predictions_validated;

        // Validate intraday predictions for all timeframes
        std::vector<TimeFrame> timeframes = {
            TimeFrame::MINUTES_15,
            TimeFrame::MINUTES_30,
            TimeFrame::HOUR_1,
            TimeFrame::HOURS_2
        };

        for (auto tf : timeframes) {
            ValidationResult intraday_result = validate_intraday_predictions(tf, "", days_back);
            combined_result.predictions_found += intraday_result.predictions_found;
            combined_result.predictions_validated += intraday_result.predictions_validated;
        }

        combined_result.success = true;
        log_info("Comprehensive validation completed: " + std::to_string(combined_result.predictions_validated) + 
                " / " + std::to_string(combined_result.predictions_found) + " total predictions validated");

    } catch (const std::exception& e) {
        combined_result.error_message = "Exception in validate_all_predictions: " + std::string(e.what());
        log_error(combined_result.error_message);
    }

    return combined_result;
}

// ==============================================
// INTERNAL VALIDATION HELPERS
// ==============================================

bool PredictionValidator::validate_daily_prediction_for_date(const std::string& symbol, const std::string& target_date) {
    try {
        // Get actual OHLC data for the target date
        auto actual_ohlc = get_actual_daily_ohlc(symbol, target_date);
        
        if (actual_ohlc.empty()) {
            log_error("No actual data found for " + symbol + " on " + target_date);
            return false;
        }

        // Update predictions_daily table with actual values and errors
        std::stringstream update_query;
        update_query << "UPDATE predictions_daily SET ";
        update_query << "actual_open = " << actual_ohlc["open"] << ", ";
        update_query << "actual_high = " << actual_ohlc["high"] << ", ";
        update_query << "actual_low = " << actual_ohlc["low"] << ", ";
        update_query << "actual_close = " << actual_ohlc["close"] << ", ";
        
        // Calculate errors
        update_query << "open_error = ABS(predicted_open - " << actual_ohlc["open"] << "), ";
        update_query << "high_error = ABS(predicted_high - " << actual_ohlc["high"] << "), ";
        update_query << "low_error = ABS(predicted_low - " << actual_ohlc["low"] << "), ";
        update_query << "close_error = ABS(predicted_close - " << actual_ohlc["close"] << "), ";
        
        // Calculate percentage errors
        update_query << "open_error_pct = ABS((predicted_open - " << actual_ohlc["open"] << ") / " << actual_ohlc["open"] << " * 100), ";
        update_query << "high_error_pct = ABS((predicted_high - " << actual_ohlc["high"] << ") / " << actual_ohlc["high"] << " * 100), ";
        update_query << "low_error_pct = ABS((predicted_low - " << actual_ohlc["low"] << ") / " << actual_ohlc["low"] << " * 100), ";
        update_query << "close_error_pct = ABS((predicted_close - " << actual_ohlc["close"] << ") / " << actual_ohlc["close"] << " * 100), ";
        
        update_query << "is_validated = TRUE, ";
        update_query << "validated_at = CURRENT_TIMESTAMP ";
        update_query << "WHERE symbol_id = " << get_symbol_id(symbol) << " ";
        update_query << "AND target_date = '" << target_date << "'";

        bool success = db_manager_->execute_query(update_query.str());
        
        if (success) {
            log_info("Validated daily prediction for " + symbol + " on " + target_date);
        } else {
            log_error("Failed to update daily prediction validation for " + symbol + " on " + target_date);
        }

        return success;

    } catch (const std::exception& e) {
        log_error("Exception validating daily prediction: " + std::string(e.what()));
        return false;
    }
}

bool PredictionValidator::validate_intraday_prediction_for_period(const std::string& symbol, 
                                                                 TimeFrame timeframe, const std::string& target_time) {
    try {
        // Get actual high/low data for the target period
        auto actual_hl = get_actual_intraday_hl(symbol, timeframe, target_time);
        
        if (actual_hl.empty()) {
            log_error("No actual data found for " + symbol + " " + timeframe_to_string(timeframe) + " at " + target_time);
            return false;
        }

        std::string timeframe_str = timeframe_to_string(timeframe);
        bool success = true;

        // Update high prediction
        if (actual_hl.find("high") != actual_hl.end()) {
            success &= update_prediction_validation(symbol, timeframe, timeframe_str + "_high", 
                                                   target_time, actual_hl["high"], 0.0);
        }

        // Update low prediction  
        if (actual_hl.find("low") != actual_hl.end()) {
            success &= update_prediction_validation(symbol, timeframe, timeframe_str + "_low", 
                                                   target_time, actual_hl["low"], 0.0);
        }

        if (success) {
            log_info("Validated intraday prediction for " + symbol + " " + timeframe_str + " at " + target_time);
        } else {
            log_error("Failed to validate intraday prediction for " + symbol + " " + timeframe_str + " at " + target_time);
        }

        return success;

    } catch (const std::exception& e) {
        log_error("Exception validating intraday prediction: " + std::string(e.what()));
        return false;
    }
}

// ==============================================
// HISTORICAL DATA RETRIEVAL FOR VALIDATION
// ==============================================

std::map<std::string, double> PredictionValidator::get_actual_daily_ohlc(const std::string& symbol, const std::string& date) {
    std::map<std::string, double> result;
    
    try {
        int symbol_id = get_symbol_id(symbol);
        if (symbol_id == -1) {
            return result;
        }

        std::stringstream query;
        query << "SELECT open_price, high_price, low_price, close_price ";
        query << "FROM historical_fetch_daily ";
        query << "WHERE symbol_id = " << symbol_id << " ";
        query << "AND fetch_date = '" << date << "'";

        PGresult* pg_result = db_manager_->execute_query_with_result(query.str());
        if (!pg_result) {
            return result;
        }

        if (PQntuples(pg_result) > 0) {
            result["open"] = std::stod(PQgetvalue(pg_result, 0, 0));
            result["high"] = std::stod(PQgetvalue(pg_result, 0, 1));
            result["low"] = std::stod(PQgetvalue(pg_result, 0, 2));
            result["close"] = std::stod(PQgetvalue(pg_result, 0, 3));
        }

        PQclear(pg_result);

    } catch (const std::exception& e) {
        log_error("Exception getting actual daily OHLC: " + std::string(e.what()));
    }

    return result;
}

std::map<std::string, double> PredictionValidator::get_actual_intraday_hl(const std::string& symbol, 
                                                                         TimeFrame timeframe, const std::string& target_time) {
    std::map<std::string, double> result;
    
    try {
        int symbol_id = get_symbol_id(symbol);
        if (symbol_id == -1) {
            return result;
        }

        std::string table_name;
        switch (timeframe) {
            case TimeFrame::MINUTES_15: table_name = "historical_fetch_15min"; break;
            case TimeFrame::MINUTES_30: table_name = "historical_fetch_30min"; break;
            case TimeFrame::HOUR_1: table_name = "historical_fetch_1hour"; break;
            case TimeFrame::HOURS_2: table_name = "historical_fetch_2hours"; break;
            default: return result;
        }

        // Parse target_time to extract date and time components
        std::stringstream query;
        query << "SELECT high_price, low_price FROM " << table_name << " ";
        query << "WHERE symbol_id = " << symbol_id << " ";
        query << "AND CONCAT(fetch_date::text, ' ', fetch_time::text)::timestamp ";
        query << "= '" << target_time << "'";

        PGresult* pg_result = db_manager_->execute_query_with_result(query.str());
        if (!pg_result) {
            return result;
        }

        if (PQntuples(pg_result) > 0) {
            result["high"] = std::stod(PQgetvalue(pg_result, 0, 0));
            result["low"] = std::stod(PQgetvalue(pg_result, 0, 1));
        }

        PQclear(pg_result);

    } catch (const std::exception& e) {
        log_error("Exception getting actual intraday HL: " + std::string(e.what()));
    }

    return result;
}

// ==============================================
// ERROR CALCULATION METHODS
// ==============================================

ErrorMetrics PredictionValidator::calculate_error_metrics(const std::vector<double>& actual_values, 
                                                         const std::vector<double>& predicted_values) {
    ErrorMetrics metrics;
    
    if (actual_values.size() != predicted_values.size() || actual_values.empty()) {
        return metrics;
    }

    metrics.sample_count = actual_values.size();
    metrics.mae = calculate_mae(actual_values, predicted_values);
    metrics.rmse = calculate_rmse(actual_values, predicted_values);
    metrics.mape = calculate_mape(actual_values, predicted_values);
    metrics.smape = calculate_smape(actual_values, predicted_values);
    metrics.r_squared = calculate_r_squared(actual_values, predicted_values);

    // Calculate max and average deviation
    std::vector<double> deviations;
    for (size_t i = 0; i < actual_values.size(); i++) {
        double dev = std::abs(actual_values[i] - predicted_values[i]);
        deviations.push_back(dev);
    }

    if (!deviations.empty()) {
        metrics.max_deviation = *std::max_element(deviations.begin(), deviations.end());
        metrics.avg_deviation = std::accumulate(deviations.begin(), deviations.end(), 0.0) / deviations.size();
    }

    return metrics;
}

double PredictionValidator::calculate_mae(const std::vector<double>& actual, const std::vector<double>& predicted) {
    double sum = 0.0;
    for (size_t i = 0; i < actual.size(); i++) {
        sum += std::abs(actual[i] - predicted[i]);
    }
    return sum / actual.size();
}

double PredictionValidator::calculate_rmse(const std::vector<double>& actual, const std::vector<double>& predicted) {
    double sum = 0.0;
    for (size_t i = 0; i < actual.size(); i++) {
        double diff = actual[i] - predicted[i];
        sum += diff * diff;
    }
    return std::sqrt(sum / actual.size());
}

double PredictionValidator::calculate_mape(const std::vector<double>& actual, const std::vector<double>& predicted) {
    double sum = 0.0;
    for (size_t i = 0; i < actual.size(); i++) {
        if (actual[i] != 0.0) {
            sum += std::abs((actual[i] - predicted[i]) / actual[i]);
        }
    }
    return (sum / actual.size()) * 100.0;
}

double PredictionValidator::calculate_smape(const std::vector<double>& actual, const std::vector<double>& predicted) {
    double sum = 0.0;
    for (size_t i = 0; i < actual.size(); i++) {
        double denominator = (std::abs(actual[i]) + std::abs(predicted[i])) / 2.0;
        if (denominator != 0.0) {
            sum += std::abs(actual[i] - predicted[i]) / denominator;
        }
    }
    return (sum / actual.size()) * 100.0;
}

double PredictionValidator::calculate_r_squared(const std::vector<double>& actual, const std::vector<double>& predicted) {
    // Calculate mean of actual values
    double mean_actual = std::accumulate(actual.begin(), actual.end(), 0.0) / actual.size();
    
    // Calculate total sum of squares and residual sum of squares
    double ss_tot = 0.0, ss_res = 0.0;
    for (size_t i = 0; i < actual.size(); i++) {
        ss_tot += (actual[i] - mean_actual) * (actual[i] - mean_actual);
        ss_res += (actual[i] - predicted[i]) * (actual[i] - predicted[i]);
    }
    
    if (ss_tot == 0.0) return 1.0; // Perfect fit case
    
    return 1.0 - (ss_res / ss_tot);
}

// ==============================================
// UTILITY METHODS
// ==============================================

int PredictionValidator::get_symbol_id(const std::string& symbol) {
    return db_manager_->get_symbol_id(symbol);
}

bool PredictionValidator::update_prediction_validation(const std::string& symbol, TimeFrame timeframe,
                                                     const std::string& prediction_type, const std::string& target_time,
                                                     double actual_value, double predicted_value) {
    try {
        int symbol_id = get_symbol_id(symbol);
        if (symbol_id == -1) {
            return false;
        }

        std::string timeframe_str = timeframe_to_string(timeframe);
        
        std::stringstream update_query;
        update_query << "UPDATE predictions_all_symbols SET ";
        update_query << "actual_value = " << actual_value << ", ";
        update_query << "absolute_error = ABS(" << actual_value << " - predicted_value), ";
        update_query << "percentage_error = ABS((" << actual_value << " - predicted_value) / " << actual_value << " * 100), ";
        update_query << "squared_error = POWER(" << actual_value << " - predicted_value, 2), ";
        update_query << "is_validated = TRUE, ";
        update_query << "validated_at = CURRENT_TIMESTAMP ";
        update_query << "WHERE symbol_id = " << symbol_id << " ";
        update_query << "AND timeframe = '" << timeframe_str << "' ";
        update_query << "AND prediction_type = '" << prediction_type << "' ";
        update_query << "AND target_time = '" << target_time << "'";

        return db_manager_->execute_query(update_query.str());

    } catch (const std::exception& e) {
        log_error("Exception updating prediction validation: " + std::string(e.what()));
        return false;
    }
}

void PredictionValidator::log_info(const std::string& message) {
    std::cout << "[INFO] PredictionValidator: " << message << std::endl;
}

void PredictionValidator::log_error(const std::string& message) {
    std::cerr << "[ERROR] PredictionValidator: " << message << std::endl;
}

void PredictionValidator::set_error(const std::string& error_message) {
    last_error_ = error_message;
    log_error(error_message);
}