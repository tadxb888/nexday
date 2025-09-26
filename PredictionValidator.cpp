#include "PredictionValidator.h"
#include "database_simple.h"
#include "IQFeedConnection/Logger.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <sstream>
#include <iomanip>
#include <chrono>

// ==============================================
// CONSTRUCTOR AND DESTRUCTOR
// ==============================================

PredictionValidator::PredictionValidator(std::shared_ptr<SimpleDatabaseManager> db_manager)
    : db_manager_(db_manager) {
    logger_ = std::make_unique<Logger>("prediction_validator.log", true);
    logger_->info("PredictionValidator initialized");
}

PredictionValidator::~PredictionValidator() = default;

// ==============================================
// MAIN VALIDATION METHODS
// ==============================================

bool PredictionValidator::validate_daily_predictions(const std::string& symbol) {
    logger_->info("Starting daily predictions validation for symbol: " + 
                 (symbol.empty() ? "ALL" : symbol));
    
    try {
        // Get unvalidated daily predictions that should have actual data available
        std::string query = 
            "SELECT p.prediction_id, p.prediction_time, p.predicted_price, p.timeframe, "
            "s.symbol, p.symbol_id "
            "FROM predictions_all_symbols p "
            "JOIN symbols s ON p.symbol_id = s.symbol_id "
            "WHERE p.is_validated = FALSE "
            "AND p.timeframe LIKE 'daily%' "
            "AND p.prediction_time < CURRENT_TIMESTAMP - INTERVAL '1 day' ";
        
        if (!symbol.empty()) {
            query += "AND s.symbol = '" + db_manager_->escape_string(symbol) + "' ";
        }
        query += "ORDER BY p.prediction_time DESC";
        
        PGresult* result = db_manager_->execute_query_with_result(query);
        if (!result) {
            logger_->error("Failed to retrieve unvalidated daily predictions");
            return false;
        }
        
        int rows = PQntuples(result);
        logger_->info("Found " + std::to_string(rows) + " unvalidated daily predictions");
        
        int validated_count = 0;
        for (int i = 0; i < rows; i++) {
            int prediction_id = std::stoi(PQgetvalue(result, i, 0));
            ValidationResult validation = validate_single_prediction(prediction_id);
            
            if (validation.is_valid) {
                if (update_prediction_validation(validation)) {
                    validated_count++;
                }
            }
        }
        
        PQclear(result);
        
        logger_->success("Validated " + std::to_string(validated_count) + 
                        " out of " + std::to_string(rows) + " daily predictions");
        return true;
        
    } catch (const std::exception& e) {
        logger_->error("Exception in validate_daily_predictions: " + std::string(e.what()));
        return false;
    }
}

bool PredictionValidator::validate_intraday_predictions(const std::string& timeframe, 
                                                       const std::string& symbol) {
    logger_->info("Starting intraday predictions validation for " + timeframe + 
                 " (symbol: " + (symbol.empty() ? "ALL" : symbol) + ")");
    
    try {
        // Calculate time threshold based on timeframe
        std::string time_threshold;
        if (timeframe == "15min") time_threshold = "20 minutes";
        else if (timeframe == "30min") time_threshold = "35 minutes";
        else if (timeframe == "1hour") time_threshold = "70 minutes";
        else if (timeframe == "2hours") time_threshold = "130 minutes";
        else {
            logger_->error("Unknown timeframe: " + timeframe);
            return false;
        }
        
        std::string query = 
            "SELECT p.prediction_id, p.prediction_time, p.predicted_price, p.timeframe, "
            "s.symbol, p.symbol_id "
            "FROM predictions_all_symbols p "
            "JOIN symbols s ON p.symbol_id = s.symbol_id "
            "WHERE p.is_validated = FALSE "
            "AND p.timeframe LIKE '" + timeframe + "%' "
            "AND p.prediction_time < CURRENT_TIMESTAMP - INTERVAL '" + time_threshold + "' ";
        
        if (!symbol.empty()) {
            query += "AND s.symbol = '" + db_manager_->escape_string(symbol) + "' ";
        }
        query += "ORDER BY p.prediction_time DESC";
        
        PGresult* result = db_manager_->execute_query_with_result(query);
        if (!result) {
            logger_->error("Failed to retrieve unvalidated " + timeframe + " predictions");
            return false;
        }
        
        int rows = PQntuples(result);
        logger_->info("Found " + std::to_string(rows) + " unvalidated " + timeframe + " predictions");
        
        int validated_count = 0;
        for (int i = 0; i < rows; i++) {
            int prediction_id = std::stoi(PQgetvalue(result, i, 0));
            ValidationResult validation = validate_single_prediction(prediction_id);
            
            if (validation.is_valid) {
                if (update_prediction_validation(validation)) {
                    validated_count++;
                }
            }
        }
        
        PQclear(result);
        
        logger_->success("Validated " + std::to_string(validated_count) + 
                        " out of " + std::to_string(rows) + " " + timeframe + " predictions");
        return true;
        
    } catch (const std::exception& e) {
        logger_->error("Exception in validate_intraday_predictions: " + std::string(e.what()));
        return false;
    }
}

bool PredictionValidator::validate_all_pending_predictions() {
    logger_->info("Starting validation of all pending predictions");
    
    bool success = true;
    
    // Validate daily predictions
    if (!validate_daily_predictions()) {
        success = false;
    }
    
    // Validate intraday predictions for all timeframes
    std::vector<std::string> timeframes = {"15min", "30min", "1hour", "2hours"};
    for (const auto& tf : timeframes) {
        if (!validate_intraday_predictions(tf)) {
            success = false;
        }
    }
    
    if (success) {
        logger_->success("All pending predictions validation completed successfully");
    } else {
        logger_->error("Some validations encountered issues");
    }
    
    return success;
}

// ==============================================
// INDIVIDUAL PREDICTION VALIDATION
// ==============================================

ValidationResult PredictionValidator::validate_single_prediction(int prediction_id) {
    ValidationResult result;
    result.prediction_id = prediction_id;
    result.is_valid = false;
    
    try {
        // Get prediction details
        std::string query = 
            "SELECT p.predicted_price, p.timeframe, p.prediction_time, s.symbol, p.symbol_id "
            "FROM predictions_all_symbols p "
            "JOIN symbols s ON p.symbol_id = s.symbol_id "
            "WHERE p.prediction_id = " + std::to_string(prediction_id);
        
        PGresult* pg_result = db_manager_->execute_query_with_result(query);
        if (!pg_result || PQntuples(pg_result) == 0) {
            logger_->error("Prediction not found: " + std::to_string(prediction_id));
            if (pg_result) PQclear(pg_result);
            return result;
        }
        
        result.predicted_price = std::stod(PQgetvalue(pg_result, 0, 0));
        result.timeframe = PQgetvalue(pg_result, 0, 1);
        std::string prediction_time = PQgetvalue(pg_result, 0, 2);
        std::string symbol = PQgetvalue(pg_result, 0, 3);
        int symbol_id = std::stoi(PQgetvalue(pg_result, 0, 4));
        
        PQclear(pg_result);
        
        // Get actual price from historical data
        result.actual_price = get_actual_price_for_prediction(prediction_id, result.timeframe, 
                                                            symbol, prediction_time);
        
        if (result.actual_price <= 0.0) {
            logger_->debug("No actual price available yet for prediction " + std::to_string(prediction_id));
            return result;
        }
        
        // Calculate error metrics
        result.prediction_error = result.actual_price - result.predicted_price;
        result.percentage_error = (std::abs(result.prediction_error) / result.actual_price) * 100.0;
        result.accuracy_score = calculate_accuracy_score(result.predicted_price, result.actual_price);
        result.is_valid = true;
        
        // Set validation timestamp
        result.validation_timestamp = get_current_timestamp();
        
        logger_->debug("Validated prediction " + std::to_string(prediction_id) + 
                      ": predicted=" + std::to_string(result.predicted_price) + 
                      ", actual=" + std::to_string(result.actual_price) + 
                      ", error=" + std::to_string(result.prediction_error) + 
                      ", accuracy=" + std::to_string(result.accuracy_score));
        
    } catch (const std::exception& e) {
        logger_->error("Exception validating prediction " + std::to_string(prediction_id) + 
                      ": " + e.what());
    }
    
    return result;
}

bool PredictionValidator::update_prediction_validation(const ValidationResult& result) {
    try {
        std::stringstream update_query;
        update_query << "UPDATE predictions_all_symbols SET ";
        update_query << "actual_price = " << result.actual_price << ", ";
        update_query << "prediction_error = " << result.prediction_error << ", ";
        update_query << "prediction_accuracy = " << result.accuracy_score << ", ";
        update_query << "is_validated = TRUE, ";
        update_query << "validated_at = '" << result.validation_timestamp << "' ";
        update_query << "WHERE prediction_id = " << result.prediction_id;
        
        bool success = db_manager_->execute_query(update_query.str());
        if (success) {
            logger_->debug("Updated validation for prediction " + std::to_string(result.prediction_id));
        }
        
        return success;
        
    } catch (const std::exception& e) {
        logger_->error("Exception updating prediction validation: " + std::string(e.what()));
        return false;
    }
}

// ==============================================
// MODEL PERFORMANCE CALCULATION
// ==============================================

ModelMetrics PredictionValidator::calculate_model_metrics(int model_id, const std::string& timeframe, 
                                                        int lookback_days) {
    ModelMetrics metrics;
    metrics.model_id = model_id;
    metrics.timeframe = timeframe;
    metrics.total_predictions = 0;
    metrics.validated_predictions = 0;
    metrics.mae = 0.0;
    metrics.rmse = 0.0;
    metrics.mape = 0.0;
    metrics.r_squared = 0.0;
    metrics.mean_accuracy = 0.0;
    metrics.std_deviation = 0.0;
    
    try {
        // Get validated predictions for this model and timeframe within lookback period
        std::string query = 
            "SELECT predicted_price, actual_price, prediction_error, prediction_accuracy "
            "FROM predictions_all_symbols "
            "WHERE model_id = " + std::to_string(model_id) + " "
            "AND timeframe LIKE '" + db_manager_->escape_string(timeframe) + "%' "
            "AND is_validated = TRUE "
            "AND prediction_time >= CURRENT_TIMESTAMP - INTERVAL '" + std::to_string(lookback_days) + " days' "
            "ORDER BY prediction_time DESC";
        
        PGresult* result = db_manager_->execute_query_with_result(query);
        if (!result) {
            logger_->error("Failed to retrieve validated predictions for model metrics");
            return metrics;
        }
        
        int rows = PQntuples(result);
        metrics.validated_predictions = rows;
        
        if (rows == 0) {
            logger_->info("No validated predictions found for model " + std::to_string(model_id) + 
                         " timeframe " + timeframe);
            PQclear(result);
            return metrics;
        }
        
        // Collect prediction and actual values
        std::vector<double> predicted_values, actual_values, errors, accuracy_scores;
        predicted_values.reserve(rows);
        actual_values.reserve(rows);
        errors.reserve(rows);
        accuracy_scores.reserve(rows);
        
        for (int i = 0; i < rows; i++) {
            double predicted = std::stod(PQgetvalue(result, i, 0));
            double actual = std::stod(PQgetvalue(result, i, 1));
            double error = std::stod(PQgetvalue(result, i, 2));
            double accuracy = std::stod(PQgetvalue(result, i, 3));
            
            predicted_values.push_back(predicted);
            actual_values.push_back(actual);
            errors.push_back(error);
            accuracy_scores.push_back(accuracy);
        }
        
        PQclear(result);
        
        // Calculate error metrics
        metrics.mae = calculate_mae(predicted_values, actual_values);
        metrics.rmse = calculate_rmse(predicted_values, actual_values);
        metrics.mape = calculate_mape(predicted_values, actual_values);
        metrics.r_squared = calculate_r_squared(predicted_values, actual_values);
        
        // Calculate mean accuracy
        metrics.mean_accuracy = std::accumulate(accuracy_scores.begin(), accuracy_scores.end(), 0.0) / accuracy_scores.size();
        
        // Calculate standard deviation of errors
        double mean_error = std::accumulate(errors.begin(), errors.end(), 0.0) / errors.size();
        double variance = 0.0;
        for (double error : errors) {
            variance += (error - mean_error) * (error - mean_error);
        }
        metrics.std_deviation = std::sqrt(variance / errors.size());
        
        // Get total predictions count (including unvalidated)
        std::string total_query = 
            "SELECT COUNT(*) FROM predictions_all_symbols "
            "WHERE model_id = " + std::to_string(model_id) + " "
            "AND timeframe LIKE '" + db_manager_->escape_string(timeframe) + "%' "
            "AND prediction_time >= CURRENT_TIMESTAMP - INTERVAL '" + std::to_string(lookback_days) + " days'";
        
        PGresult* total_result = db_manager_->execute_query_with_result(total_query);
        if (total_result && PQntuples(total_result) > 0) {
            metrics.total_predictions = std::stoi(PQgetvalue(total_result, 0, 0));
        }
        if (total_result) PQclear(total_result);
        
        logger_->info("Calculated metrics for model " + std::to_string(model_id) + 
                     " " + timeframe + ": MAE=" + std::to_string(metrics.mae) + 
                     ", RMSE=" + std::to_string(metrics.rmse) + 
                     ", R²=" + std::to_string(metrics.r_squared));
        
    } catch (const std::exception& e) {
        logger_->error("Exception calculating model metrics: " + std::string(e.what()));
    }
    
    return metrics;
}

bool PredictionValidator::update_model_performance(const ModelMetrics& metrics) {
    try {
        // Update the accuracy_metrics JSONB field in model_standard table
        std::stringstream jsonb_metrics;
        jsonb_metrics << "'{";
        jsonb_metrics << "\"mae\": " << metrics.mae << ", ";
        jsonb_metrics << "\"rmse\": " << metrics.rmse << ", ";
        jsonb_metrics << "\"mape\": " << metrics.mape << ", ";
        jsonb_metrics << "\"r_squared\": " << metrics.r_squared << ", ";
        jsonb_metrics << "\"mean_accuracy\": " << metrics.mean_accuracy << ", ";
        jsonb_metrics << "\"std_deviation\": " << metrics.std_deviation << ", ";
        jsonb_metrics << "\"total_predictions\": " << metrics.total_predictions << ", ";
        jsonb_metrics << "\"validated_predictions\": " << metrics.validated_predictions << ", ";
        jsonb_metrics << "\"timeframe\": \"" << metrics.timeframe << "\", ";
        jsonb_metrics << "\"last_calculated\": \"" << get_current_timestamp() << "\"";
        jsonb_metrics << "}'";
        
        std::string update_query = 
            "UPDATE model_standard SET "
            "accuracy_metrics = " + jsonb_metrics.str() + ", "
            "updated_at = CURRENT_TIMESTAMP, "
            "last_validated = CURRENT_TIMESTAMP "
            "WHERE model_id = " + std::to_string(metrics.model_id);
        
        bool success = db_manager_->execute_query(update_query);
        
        if (success) {
            logger_->success("Updated performance metrics for model " + std::to_string(metrics.model_id));
            
            // Also update model_std_deviation table for statistical tracking
            update_model_standard_deviation(metrics);
        }
        
        return success;
        
    } catch (const std::exception& e) {
        logger_->error("Exception updating model performance: " + std::string(e.what()));
        return false;
    }
}

bool PredictionValidator::update_model_standard_deviation(const ModelMetrics& metrics) {
    try {
        // Insert or update model_std_deviation record
        std::string upsert_query = 
            "INSERT INTO model_std_deviation "
            "(model_id, symbol_id, timeframe, std_deviation, sample_size, last_calculated) "
            "SELECT " + std::to_string(metrics.model_id) + ", symbol_id, '" + 
            db_manager_->escape_string(metrics.timeframe) + "', " + 
            std::to_string(metrics.std_deviation) + ", " + 
            std::to_string(metrics.validated_predictions) + ", CURRENT_TIMESTAMP "
            "FROM symbols WHERE symbol = 'QGC#' LIMIT 1 "
            "ON CONFLICT (model_id, symbol_id, timeframe) DO UPDATE SET "
            "std_deviation = EXCLUDED.std_deviation, "
            "sample_size = EXCLUDED.sample_size, "
            "last_calculated = EXCLUDED.last_calculated";
        
        return db_manager_->execute_query(upsert_query);
        
    } catch (const std::exception& e) {
        logger_->error("Exception updating model standard deviation: " + std::string(e.what()));
        return false;
    }
}

// ==============================================
// UTILITY METHODS
// ==============================================

std::vector<int> PredictionValidator::get_unvalidated_predictions(const std::string& timeframe) {
    std::vector<int> prediction_ids;
    
    try {
        std::string query = "SELECT prediction_id FROM predictions_all_symbols WHERE is_validated = FALSE";
        if (!timeframe.empty()) {
            query += " AND timeframe LIKE '" + db_manager_->escape_string(timeframe) + "%'";
        }
        query += " ORDER BY prediction_time DESC";
        
        PGresult* result = db_manager_->execute_query_with_result(query);
        if (!result) {
            return prediction_ids;
        }
        
        int rows = PQntuples(result);
        prediction_ids.reserve(rows);
        
        for (int i = 0; i < rows; i++) {
            prediction_ids.push_back(std::stoi(PQgetvalue(result, i, 0)));
        }
        
        PQclear(result);
        
    } catch (const std::exception& e) {
        logger_->error("Exception getting unvalidated predictions: " + std::string(e.what()));
    }
    
    return prediction_ids;
}

double PredictionValidator::get_actual_price_for_prediction(int prediction_id, 
                                                          const std::string& timeframe,
                                                          const std::string& symbol, 
                                                          const std::string& prediction_time) {
    try {
        std::string table_name;
        std::string time_column = "fetch_date";
        
        // Determine which historical table to query
        if (timeframe.find("daily") != std::string::npos) {
            table_name = "historical_fetch_daily";
            // For daily predictions, get the next business day's close price
        } else if (timeframe.find("15min") != std::string::npos) {
            table_name = "historical_fetch_15min";
            time_column = "fetch_date, fetch_time";
        } else if (timeframe.find("30min") != std::string::npos) {
            table_name = "historical_fetch_30min";
            time_column = "fetch_date, fetch_time";
        } else if (timeframe.find("1hour") != std::string::npos || timeframe.find("1h") != std::string::npos) {
            table_name = "historical_fetch_1hour";
            time_column = "fetch_date, fetch_time";
        } else if (timeframe.find("2hour") != std::string::npos) {
            table_name = "historical_fetch_2hours";
            time_column = "fetch_date, fetch_time";
        } else {
            logger_->error("Unknown timeframe for actual price lookup: " + timeframe);
            return 0.0;
        }
        
        // Build query to get actual price
        std::stringstream query;
        query << "SELECT ";
        
        // Determine which price column to use based on prediction type
        if (timeframe.find("_high") != std::string::npos) {
            query << "high_price ";
        } else if (timeframe.find("_low") != std::string::npos) {
            query << "low_price ";
        } else if (timeframe.find("_open") != std::string::npos) {
            query << "open_price ";
        } else {
            query << "close_price "; // Default to close price
        }
        
        query << "FROM " << table_name << " h ";
        query << "JOIN symbols s ON h.symbol_id = s.symbol_id ";
        query << "WHERE s.symbol = '" << db_manager_->escape_string(symbol) << "' ";
        
        // Add time constraint based on prediction time
        if (table_name == "historical_fetch_daily") {
            // For daily, get next business day
            query << "AND h.fetch_date > DATE('" << prediction_time.substr(0, 10) << "') ";
            query << "ORDER BY h.fetch_date ASC LIMIT 1";
        } else {
            // For intraday, get the specific time period that was predicted
            query << "AND (h.fetch_date || ' ' || h.fetch_time)::timestamp > '" << prediction_time << "' ";
            query << "ORDER BY h.fetch_date ASC, h.fetch_time ASC LIMIT 1";
        }
        
        PGresult* result = db_manager_->execute_query_with_result(query.str());
        if (!result || PQntuples(result) == 0) {
            if (result) PQclear(result);
            return 0.0; // No actual data available yet
        }
        
        double actual_price = std::stod(PQgetvalue(result, 0, 0));
        PQclear(result);
        
        return actual_price;
        
    } catch (const std::exception& e) {
        logger_->error("Exception getting actual price: " + std::string(e.what()));
        return 0.0;
    }
}

std::string PredictionValidator::get_current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

// ==============================================
// ERROR CALCULATION FUNCTIONS (STATIC)
// ==============================================

double PredictionValidator::calculate_mae(const std::vector<double>& predicted, 
                                        const std::vector<double>& actual) {
    if (predicted.size() != actual.size() || predicted.empty()) return 0.0;
    
    double sum = 0.0;
    for (size_t i = 0; i < predicted.size(); ++i) {
        sum += std::abs(actual[i] - predicted[i]);
    }
    return sum / predicted.size();
}

double PredictionValidator::calculate_rmse(const std::vector<double>& predicted, 
                                         const std::vector<double>& actual) {
    if (predicted.size() != actual.size() || predicted.empty()) return 0.0;
    
    double sum = 0.0;
    for (size_t i = 0; i < predicted.size(); ++i) {
        double error = actual[i] - predicted[i];
        sum += error * error;
    }
    return std::sqrt(sum / predicted.size());
}

double PredictionValidator::calculate_mape(const std::vector<double>& predicted, 
                                         const std::vector<double>& actual) {
    if (predicted.size() != actual.size() || predicted.empty()) return 0.0;
    
    double sum = 0.0;
    for (size_t i = 0; i < predicted.size(); ++i) {
        if (actual[i] != 0.0) {
            sum += std::abs((actual[i] - predicted[i]) / actual[i]);
        }
    }
    return (sum / predicted.size()) * 100.0;
}

double PredictionValidator::calculate_r_squared(const std::vector<double>& predicted, 
                                              const std::vector<double>& actual) {
    if (predicted.size() != actual.size() || predicted.empty()) return 0.0;
    
    // Calculate mean of actual values
    double mean_actual = std::accumulate(actual.begin(), actual.end(), 0.0) / actual.size();
    
    // Calculate total sum of squares and residual sum of squares
    double ss_tot = 0.0, ss_res = 0.0;
    for (size_t i = 0; i < actual.size(); ++i) {
        ss_tot += (actual[i] - mean_actual) * (actual[i] - mean_actual);
        ss_res += (actual[i] - predicted[i]) * (actual[i] - predicted[i]);
    }
    
    if (ss_tot == 0.0) return 0.0;
    return 1.0 - (ss_res / ss_tot);
}

double PredictionValidator::calculate_accuracy_score(double predicted, double actual) {
    if (actual == 0.0) return 0.0;
    
    double percentage_error = std::abs((actual - predicted) / actual) * 100.0;
    // Convert to accuracy score (0-1 scale, 1 = perfect)
    return std::max(0.0, 1.0 - (percentage_error / 100.0));
}

// ==============================================
// REPORTING METHODS
// ==============================================

void PredictionValidator::print_validation_summary(const std::string& timeframe) {
    try {
        std::string where_clause;
        if (!timeframe.empty()) {
            where_clause = "WHERE timeframe LIKE '" + db_manager_->escape_string(timeframe) + "%'";
        }
        
        std::string summary_query = 
            "SELECT "
            "  timeframe, "
            "  COUNT(*) as total, "
            "  COUNT(*) FILTER (WHERE is_validated = TRUE) as validated, "
            "  AVG(prediction_accuracy) FILTER (WHERE is_validated = TRUE) as avg_accuracy, "
            "  AVG(ABS(prediction_error)) FILTER (WHERE is_validated = TRUE) as avg_abs_error "
            "FROM predictions_all_symbols " + where_clause + " "
            "GROUP BY timeframe "
            "ORDER BY timeframe";
        
        PGresult* result = db_manager_->execute_query_with_result(summary_query);
        if (!result) {
            std::cout << "Failed to retrieve validation summary" << std::endl;
            return;
        }
        
        int rows = PQntuples(result);
        if (rows == 0) {
            std::cout << "No predictions found for validation summary" << std::endl;
            PQclear(result);
            return;
        }
        
        std::cout << "\n=== PREDICTION VALIDATION SUMMARY ===" << std::endl;
        std::cout << std::setw(15) << "Timeframe" 
                  << std::setw(10) << "Total" 
                  << std::setw(12) << "Validated" 
                  << std::setw(15) << "Avg Accuracy" 
                  << std::setw(15) << "Avg Abs Error" << std::endl;
        std::cout << std::string(67, '-') << std::endl;
        
        for (int i = 0; i < rows; i++) {
            std::string tf = PQgetvalue(result, i, 0);
            int total = std::stoi(PQgetvalue(result, i, 1));
            int validated = std::stoi(PQgetvalue(result, i, 2));
            
            std::cout << std::setw(15) << tf
                      << std::setw(10) << total
                      << std::setw(12) << validated;
            
            if (validated > 0) {
                double avg_accuracy = std::stod(PQgetvalue(result, i, 3));
                double avg_error = std::stod(PQgetvalue(result, i, 4));
                
                std::cout << std::setw(14) << std::fixed << std::setprecision(2) 
                          << (avg_accuracy * 100) << "%"
                          << std::setw(15) << std::fixed << std::setprecision(2) 
                          << avg_error;
            } else {
                std::cout << std::setw(15) << "N/A" << std::setw(15) << "N/A";
            }
            std::cout << std::endl;
        }
        
        PQclear(result);
        std::cout << "======================================" << std::endl;
        
    } catch (const std::exception& e) {
        logger_->error("Exception in print_validation_summary: " + std::string(e.what()));
        std::cout << "Error displaying validation summary" << std::endl;
    }
}

void PredictionValidator::print_model_performance(int model_id) {
    try {
        std::cout << "\n=== MODEL PERFORMANCE METRICS ===" << std::endl;
        std::cout << "Model ID: " << model_id << std::endl;
        std::cout << "=================================" << std::endl;
        
        std::vector<std::string> timeframes = {"daily", "15min", "30min", "1hour", "2hours"};
        
        for (const auto& tf : timeframes) {
            ModelMetrics metrics = calculate_model_metrics(model_id, tf, 30); // Last 30 days
            
            std::cout << "\n" << tf << " Predictions:" << std::endl;
            std::cout << "  Total Predictions: " << metrics.total_predictions << std::endl;
            std::cout << "  Validated: " << metrics.validated_predictions << std::endl;
            
            if (metrics.validated_predictions > 0) {
                std::cout << "  MAE: " << std::fixed << std::setprecision(4) << metrics.mae << std::endl;
                std::cout << "  RMSE: " << std::fixed << std::setprecision(4) << metrics.rmse << std::endl;
                std::cout << "  MAPE: " << std::fixed << std::setprecision(2) << metrics.mape << "%" << std::endl;
                std::cout << "  R-squared: " << std::fixed << std::setprecision(4) << metrics.r_squared << std::endl;
                std::cout << "  Mean Accuracy: " << std::fixed << std::setprecision(2) << metrics.mean_accuracy * 100 << "%" << std::endl;
                std::cout << "  Std Deviation: " << std::fixed << std::setprecision(4) << metrics.std_deviation << std::endl;
            } else {
                std::cout << "  No validated predictions available" << std::endl;
            }
        }
        
        std::cout << "\n=================================" << std::endl;
        
    } catch (const std::exception& e) {
        logger_->error("Exception in print_model_performance: " + std::string(e.what()));
        std::cout << "Error displaying model performance" << std::endl;
    }
}