#include "IntegratedMarketPredictionEngine.h"
#include "database_simple.h"
#include "IQFeedConnectionManager.h"
#include "Logger.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <ctime>
#include <stdexcept>
#include <libpq-fe.h>

IntegratedMarketPredictionEngine::IntegratedMarketPredictionEngine(
    std::shared_ptr<SimpleDatabaseManager> db_manager,
    std::shared_ptr<IQFeedConnectionManager> iqfeed_manager)
    : db_manager_(db_manager), iqfeed_manager_(iqfeed_manager) {
    
    logger_ = std::make_unique<Logger>("prediction_engine_integrated.log", true);
    logger_->info("Integrated Market Prediction Engine initialized");
    logger_->info("Using Model 1 Standard: base_alpha=" + std::to_string(BASE_ALPHA) + 
                 ", min_bars=" + std::to_string(MINIMUM_BARS));
}

IntegratedMarketPredictionEngine::~IntegratedMarketPredictionEngine() {
    logger_->info("Integrated Market Prediction Engine shutting down");
}

// ==============================================
// MAIN PREDICTION METHODS
// ==============================================

bool IntegratedMarketPredictionEngine::generate_predictions_for_symbol(const std::string& symbol) {
    logger_->info("Generating comprehensive predictions for symbol: " + symbol);
    
    if (!is_ready()) {
        logger_->error("Prediction engine not ready - database or IQFeed connection issue");
        return false;
    }
    
    bool overall_success = true;
    
    // Generate daily predictions
    if (!generate_daily_prediction(symbol)) {
        logger_->error("Failed to generate daily prediction for " + symbol);
        overall_success = false;
    }
    
    // Generate intraday predictions for multiple timeframes
    std::vector<std::string> intraday_timeframes = {"15min", "30min", "1hour", "2hours"};
    for (const auto& timeframe : intraday_timeframes) {
        if (!generate_intraday_prediction(symbol, timeframe)) {
            logger_->error("Failed to generate " + timeframe + " prediction for " + symbol);
            overall_success = false;
        }
    }
    
    if (overall_success) {
        logger_->success("Successfully generated all predictions for " + symbol);
    }
    
    return overall_success;
}

bool IntegratedMarketPredictionEngine::generate_daily_prediction(const std::string& symbol) {
    logger_->info("Generating daily prediction for: " + symbol);
    
    try {
        // Step 1: Retrieve historical daily data from database
        std::vector<PriceBar> historical_data;
        if (!retrieve_historical_data_from_db(symbol, "daily", historical_data)) {
            logger_->error("Failed to retrieve historical daily data from database for " + symbol);
            return false;
        }
        
        logger_->info("Retrieved " + std::to_string(historical_data.size()) + " daily bars from database");
        
        // Step 2: Validate we have sufficient data
        if (historical_data.size() < MINIMUM_BARS) {
            // Try to fetch fresh data from IQFeed
            logger_->info("Insufficient database data (" + std::to_string(historical_data.size()) + 
                         " bars). Fetching from IQFeed...");
            
            if (!fetch_fresh_data_from_iqfeed(symbol, "daily", 100, historical_data)) {
                logger_->error("Failed to fetch sufficient historical data from IQFeed");
                return false;
            }
        }
        
        // Step 3: Generate predictions using EMA Model 1 Standard
        PredictionResult result;
        if (!generate_daily_predictions(symbol, historical_data, result)) {
            logger_->error("Failed to generate daily predictions for " + symbol);
            return false;
        }
        
        // Step 4: Save predictions to database
        if (!save_predictions_to_db(result)) {
            logger_->error("Failed to save daily predictions to database for " + symbol);
            return false;
        }
        
        // Step 5: Log prediction summary
        print_prediction_summary(result);
        
        return true;
        
    } catch (const std::exception& e) {
        handle_prediction_error("daily prediction for " + symbol, e.what());
        return false;
    }
}

bool IntegratedMarketPredictionEngine::generate_intraday_prediction(const std::string& symbol, 
                                                                   const std::string& timeframe) {
    logger_->info("Generating " + timeframe + " prediction for: " + symbol);
    
    try {
        // Step 1: Retrieve historical intraday data
        std::vector<PriceBar> historical_data;
        if (!retrieve_historical_data_from_db(symbol, timeframe, historical_data)) {
            logger_->error("Failed to retrieve " + timeframe + " data from database");
            
            // Fallback to IQFeed
            if (!fetch_fresh_data_from_iqfeed(symbol, timeframe, 100, historical_data)) {
                logger_->error("Failed to fetch " + timeframe + " data from IQFeed");
                return false;
            }
        }
        
        logger_->info("Retrieved " + std::to_string(historical_data.size()) + " " + timeframe + " bars");
        
        // Step 2: Generate intraday predictions
        PredictionResult result;
        if (!generate_intraday_predictions(symbol, timeframe, historical_data, result)) {
            logger_->error("Failed to generate " + timeframe + " predictions");
            return false;
        }
        
        // Step 3: Save to database
        if (!save_predictions_to_db(result)) {
            logger_->error("Failed to save " + timeframe + " predictions to database");
            return false;
        }
        
        logger_->success(timeframe + " prediction generated for " + symbol + 
                        ": High=" + std::to_string(result.predicted_next_high) + 
                        ", Low=" + std::to_string(result.predicted_next_low));
        
        return true;
        
    } catch (const std::exception& e) {
        handle_prediction_error(timeframe + " prediction for " + symbol, e.what());
        return false;
    }
}

// ==============================================
// DATA RETRIEVAL METHODS
// ==============================================

bool IntegratedMarketPredictionEngine::retrieve_historical_data_from_db(const std::string& symbol, 
                                                                        const std::string& timeframe,
                                                                        std::vector<PriceBar>& historical_data) {
    try {
        // Get symbol ID
        int symbol_id = get_or_create_symbol_id(symbol);
        if (symbol_id == -1) {
            logger_->error("Failed to get symbol ID for: " + symbol);
            return false;
        }
        
        // Build query based on timeframe
        std::string table_name;
        std::string date_column;
        
        if (timeframe == "daily") {
            table_name = "historical_fetch_daily";
            date_column = "fetch_date";  // Daily table uses fetch_date
        } else if (timeframe == "15min") {
            table_name = "historical_fetch_15min";
            date_column = "fetch_date, fetch_time";  // Intraday tables have both date and time
        } else if (timeframe == "30min") {
            table_name = "historical_fetch_30min";
            date_column = "fetch_date, fetch_time";
        } else if (timeframe == "1hour") {
            table_name = "historical_fetch_1hour";
            date_column = "fetch_date, fetch_time";
        } else if (timeframe == "2hours") {
            table_name = "historical_fetch_2hours";
            date_column = "fetch_date, fetch_time";
        } else {
            logger_->error("Unsupported timeframe: " + timeframe);
            return false;
        }
        
        // Construct SQL query to get recent historical data (newest first)
        std::stringstream query;
        query << "SELECT " << date_column << ", open_price, high_price, low_price, close_price, volume "
              << "FROM " << table_name << " "
              << "WHERE symbol_id = " << symbol_id << " "
              << "ORDER BY ";
        
        if (timeframe == "daily") {
            query << "fetch_date DESC ";
        } else {
            query << "fetch_date DESC, fetch_time DESC ";
        }
        
        query << "LIMIT 100"; // Get last 100 bars
        
        logger_->debug("Executing query: " + query.str());
        
        // Execute query using SimpleDatabaseManager (now that methods are public)
        PGresult* result = db_manager_->execute_query_with_result(query.str());
        if (!result) {
            logger_->error("Database query failed for " + symbol + " " + timeframe);
            return false;
        }
        
        int rows = PQntuples(result);
        logger_->debug("Query returned " + std::to_string(rows) + " rows");
        
        historical_data.clear();
        historical_data.reserve(rows);
        
        for (int i = 0; i < rows; i++) {
            PriceBar bar;
            
            if (timeframe == "daily") {
                // Daily data: date, open, high, low, close, volume
                bar.date = PQgetvalue(result, i, 0);
                bar.time = "";  // No time for daily
                bar.open = std::stod(PQgetvalue(result, i, 1));
                bar.high = std::stod(PQgetvalue(result, i, 2));
                bar.low = std::stod(PQgetvalue(result, i, 3));
                bar.close = std::stod(PQgetvalue(result, i, 4));
                bar.volume = std::stoi(PQgetvalue(result, i, 5));
            } else {
                // Intraday data: date, time, open, high, low, close, volume
                bar.date = PQgetvalue(result, i, 0);
                bar.time = PQgetvalue(result, i, 1);
                bar.open = std::stod(PQgetvalue(result, i, 2));
                bar.high = std::stod(PQgetvalue(result, i, 3));
                bar.low = std::stod(PQgetvalue(result, i, 4));
                bar.close = std::stod(PQgetvalue(result, i, 5));
                bar.volume = std::stoi(PQgetvalue(result, i, 6));
            }
            
            historical_data.push_back(bar);
        }
        
        PQclear(result);
        
        logger_->info("Successfully retrieved " + std::to_string(historical_data.size()) + 
                     " " + timeframe + " bars from database for " + symbol);
        
        // Data comes back newest first, but we need oldest first for EMA calculations
        std::reverse(historical_data.begin(), historical_data.end());
        
        return !historical_data.empty();
        
    } catch (const std::exception& e) {
        logger_->error("Exception in retrieve_historical_data_from_db: " + std::string(e.what()));
        return false;
    }
}

bool IntegratedMarketPredictionEngine::fetch_fresh_data_from_iqfeed(const std::string& symbol, 
                                                                   const std::string& timeframe,
                                                                   int num_bars, 
                                                                   std::vector<PriceBar>& fresh_data) {
    logger_->info("Fetching fresh " + timeframe + " data from IQFeed for " + symbol);
    
    // This would integrate with your existing IQFeed fetchers
    // For now, we'll return false and rely on database data
    // In a full implementation, you'd use your existing fetcher classes:
    
    /*
    if (timeframe == "daily") {
        auto daily_fetcher = std::make_unique<DailyDataFetcher>(iqfeed_manager_);
        std::vector<HistoricalBar> iqfeed_bars;
        
        if (daily_fetcher->fetch_historical_data(symbol, num_bars, iqfeed_bars)) {
            // Convert HistoricalBar to PriceBar format
            fresh_data.clear();
            for (const auto& bar : iqfeed_bars) {
                PriceBar price_bar;
                price_bar.date = bar.date;
                price_bar.time = bar.time;
                price_bar.open = bar.open;
                price_bar.high = bar.high;
                price_bar.low = bar.low;
                price_bar.close = bar.close;
                price_bar.volume = bar.volume;
                fresh_data.push_back(price_bar);
            }
            return true;
        }
    }
    */
    
    logger_->info("IQFeed fallback not implemented yet - relying on database data");
    return false;
}

// ==============================================
// EMA CALCULATION METHODS
// ==============================================

EMAResult IntegratedMarketPredictionEngine::calculate_ema_sequence(const std::vector<PriceBar>& price_data, 
                                                                  const std::string& price_type) {
    EMAResult result;
    
    if (price_data.size() < MINIMUM_BARS) {
        result.error_message = "Insufficient data: need " + std::to_string(MINIMUM_BARS) + 
                              " bars, have " + std::to_string(price_data.size());
        return result;
    }
    
    // Step 1: Calculate SMA10 (bootstrap) from first 10 bars
    result.sma10 = calculate_sma_10(price_data, price_type);
    if (result.sma10 == 0) {
        result.error_message = "Failed to calculate SMA10 bootstrap";
        return result;
    }
    
    logger_->debug("Bootstrap SMA10: " + std::to_string(result.sma10));
    
    // Step 2: Initialize EMA sequence
    result.ema_sequence.clear();
    result.ema_sequence.reserve(price_data.size() - BOOTSTRAP_BARS);
    
    double previous_ema = result.sma10;  // SMA10 becomes initial previous_predict
    
    // Step 3: Calculate EMA sequence starting from bar 11 (index 10)
    for (size_t i = BOOTSTRAP_BARS; i < price_data.size(); i++) {
        double current_value;
        
        // Extract the requested price type
        if (price_type == "open") current_value = price_data[i].open;
        else if (price_type == "high") current_value = price_data[i].high;
        else if (price_type == "low") current_value = price_data[i].low;
        else current_value = price_data[i].close;  // default to close
        
        // EMA Formula: predict_t = (base_alpha * current_value) + ((1 - base_alpha) * previous_predict)
        double current_ema = (BASE_ALPHA * current_value) + ((1.0 - BASE_ALPHA) * previous_ema);
        
        result.ema_sequence.push_back(current_ema);
        previous_ema = current_ema;  // Current EMA becomes previous for next iteration
        
        logger_->debug("EMA[" + std::to_string(i - BOOTSTRAP_BARS + 1) + "]: " + std::to_string(current_ema) + 
                      " (current=" + std::to_string(current_value) + ", prev=" + std::to_string(previous_ema) + ")");
    }
    
    // Step 4: Set final EMA as the last calculated value
    result.final_ema = previous_ema;
    result.calculation_valid = true;
    
    logger_->info("EMA calculation completed. Final EMA: " + std::to_string(result.final_ema) + 
                 " (sequence length: " + std::to_string(result.ema_sequence.size()) + ")");
    
    return result;
}

double IntegratedMarketPredictionEngine::calculate_sma_10(const std::vector<PriceBar>& price_data, 
                                                         const std::string& price_type) {
    if (price_data.size() < BOOTSTRAP_BARS) {
        logger_->error("Insufficient data for SMA10 calculation");
        return 0.0;
    }
    
    double sum = 0.0;
    for (int i = 0; i < BOOTSTRAP_BARS; i++) {
        double value;
        if (price_type == "open") value = price_data[i].open;
        else if (price_type == "high") value = price_data[i].high;
        else if (price_type == "low") value = price_data[i].low;
        else value = price_data[i].close;
        
        sum += value;
    }
    
    double sma10 = sum / BOOTSTRAP_BARS;
    logger_->debug("SMA10 calculated: " + std::to_string(sma10) + " from " + std::to_string(BOOTSTRAP_BARS) + " bars");
    
    return sma10;
}

// ==============================================
// PREDICTION GENERATION METHODS
// ==============================================

bool IntegratedMarketPredictionEngine::generate_daily_predictions(const std::string& symbol,
                                                                 const std::vector<PriceBar>& historical_data,
                                                                 PredictionResult& result) {
    logger_->info("Generating daily OHLC predictions for " + symbol);
    
    result.symbol = symbol;
    result.timeframe = "daily";
    result.bars_used = historical_data.size();
    result.base_alpha = BASE_ALPHA;
    result.created_at = std::chrono::system_clock::now();
    
    // Calculate next business day
    if (!historical_data.empty()) {
        result.prediction_date = get_next_business_day(historical_data.back().date);
    }
    
    try {
        // Generate predictions for each OHLC component
        EMAResult open_ema = calculate_ema_sequence(historical_data, "open");
        EMAResult high_ema = calculate_ema_sequence(historical_data, "high");
        EMAResult low_ema = calculate_ema_sequence(historical_data, "low");
        EMAResult close_ema = calculate_ema_sequence(historical_data, "close");
        
        // Validate all calculations
        if (!open_ema.calculation_valid || !high_ema.calculation_valid || 
            !low_ema.calculation_valid || !close_ema.calculation_valid) {
            result.error_message = "EMA calculation failed for one or more OHLC components";
            return false;
        }
        
        // Set predictions
        result.predicted_open = open_ema.final_ema;
        result.predicted_high = high_ema.final_ema;
        result.predicted_low = low_ema.final_ema;
        result.predicted_close = close_ema.final_ema;
        
        // Calculate confidence based on data quality
        result.confidence_score = std::min(0.95, static_cast<double>(historical_data.size()) / 100.0);
        result.prediction_valid = true;
        
        logger_->info("Daily predictions generated for " + symbol + " (next business day: " + result.prediction_date + "):");
        logger_->info("  Predicted Open: " + std::to_string(result.predicted_open));
        logger_->info("  Predicted High: " + std::to_string(result.predicted_high));
        logger_->info("  Predicted Low: " + std::to_string(result.predicted_low));
        logger_->info("  Predicted Close: " + std::to_string(result.predicted_close));
        
        return true;
        
    } catch (const std::exception& e) {
        result.error_message = e.what();
        result.prediction_valid = false;
        return false;
    }
}

bool IntegratedMarketPredictionEngine::generate_intraday_predictions(const std::string& symbol,
                                                                    const std::string& timeframe,
                                                                    const std::vector<PriceBar>& historical_data,
                                                                    PredictionResult& result) {
    logger_->info("Generating " + timeframe + " High/Low predictions for " + symbol);
    
    result.symbol = symbol;
    result.timeframe = timeframe;
    result.bars_used = historical_data.size();
    result.base_alpha = BASE_ALPHA;
    result.created_at = std::chrono::system_clock::now();
    
    try {
        // Generate High and Low predictions for next interval
        EMAResult high_ema = calculate_ema_sequence(historical_data, "high");
        EMAResult low_ema = calculate_ema_sequence(historical_data, "low");
        
        if (!high_ema.calculation_valid || !low_ema.calculation_valid) {
            result.error_message = "EMA calculation failed for High/Low predictions";
            return false;
        }
        
        result.predicted_next_high = high_ema.final_ema;
        result.predicted_next_low = low_ema.final_ema;
        
        // Calculate confidence
        result.confidence_score = std::min(0.90, static_cast<double>(historical_data.size()) / 100.0);
        result.prediction_valid = true;
        
        logger_->info(timeframe + " predictions for " + symbol + ":");
        logger_->info("  Next " + timeframe + " High: " + std::to_string(result.predicted_next_high));
        logger_->info("  Next " + timeframe + " Low: " + std::to_string(result.predicted_next_low));
        
        return true;
        
    } catch (const std::exception& e) {
        result.error_message = e.what();
        result.prediction_valid = false;
        return false;
    }
}

// ==============================================
// DATABASE OPERATIONS
// ==============================================

int IntegratedMarketPredictionEngine::get_or_create_symbol_id(const std::string& symbol) {
    return db_manager_->get_or_create_symbol_id(symbol);
}

int IntegratedMarketPredictionEngine::get_or_create_model_id() {
    // This should create/get the "Epoch Market Advisor" model entry
    // For now, return a fixed ID - in full implementation, create proper model entry
    return 1;
}

bool IntegratedMarketPredictionEngine::save_predictions_to_db(const PredictionResult& result) {
    logger_->info("Saving predictions to database for " + result.symbol + " " + result.timeframe);
    
    try {
        int symbol_id = get_or_create_symbol_id(result.symbol);
        int model_id = get_or_create_model_id();
        
        if (symbol_id == -1 || model_id == -1) {
            logger_->error("Failed to get symbol_id or model_id");
            return false;
        }
        
        // Save to predictions_all_symbols table
        std::stringstream insert_query;
        
        if (result.timeframe == "daily") {
            // Save daily OHLC predictions
            std::vector<std::pair<std::string, double>> predictions = {
                {"daily_open", result.predicted_open},
                {"daily_high", result.predicted_high},
                {"daily_low", result.predicted_low},
                {"daily_close", result.predicted_close}
            };
            
            for (const auto& pred : predictions) {
                insert_query.str("");
                insert_query << "INSERT INTO predictions_all_symbols (";
                insert_query << "prediction_time, symbol_id, model_id, timeframe, predicted_price, ";
                insert_query << "confidence_score, prediction_horizon, current_price";
                insert_query << ") VALUES (";
                insert_query << "NOW(), " << symbol_id << ", " << model_id << ", ";
                insert_query << "'" << pred.first << "', " << pred.second << ", ";
                insert_query << result.confidence_score << ", 1440, ";  // 1440 minutes = 1 day
                insert_query << "0.0";  // current_price - would need to fetch current price
                insert_query << ") ON CONFLICT (prediction_time, symbol_id, model_id, timeframe) DO UPDATE SET ";
                insert_query << "predicted_price = EXCLUDED.predicted_price, ";
                insert_query << "confidence_score = EXCLUDED.confidence_score";
                
                if (!db_manager_->execute_query(insert_query.str())) {
                    logger_->error("Failed to save " + pred.first + " prediction");
                    return false;
                }
            }
        } else {
            // Save intraday High/Low predictions
            std::vector<std::pair<std::string, double>> predictions = {
                {result.timeframe + "_high", result.predicted_next_high},
                {result.timeframe + "_low", result.predicted_next_low}
            };
            
            for (const auto& pred : predictions) {
                insert_query.str("");
                insert_query << "INSERT INTO predictions_all_symbols (";
                insert_query << "prediction_time, symbol_id, model_id, timeframe, predicted_price, ";
                insert_query << "confidence_score, prediction_horizon, current_price";
                insert_query << ") VALUES (";
                insert_query << "NOW(), " << symbol_id << ", " << model_id << ", ";
                insert_query << "'" << pred.first << "', " << pred.second << ", ";
                insert_query << result.confidence_score << ", 60, ";  // Varies by timeframe
                insert_query << "0.0";
                insert_query << ") ON CONFLICT (prediction_time, symbol_id, model_id, timeframe) DO UPDATE SET ";
                insert_query << "predicted_price = EXCLUDED.predicted_price, ";
                insert_query << "confidence_score = EXCLUDED.confidence_score";
                
                if (!db_manager_->execute_query(insert_query.str())) {
                    logger_->error("Failed to save " + pred.first + " prediction");
                    return false;
                }
            }
        }
        
        logger_->success("Successfully saved " + result.timeframe + " predictions for " + result.symbol);
        return true;
        
    } catch (const std::exception& e) {
        logger_->error("Exception saving predictions: " + std::string(e.what()));
        return false;
    }
}

// ==============================================
// UTILITY METHODS
// ==============================================

std::string IntegratedMarketPredictionEngine::get_next_business_day(const std::string& from_date) {
    // Simple implementation - add 1 day and handle weekends
    // In full implementation, handle holidays too
    
    std::tm tm = {};
    std::istringstream ss(from_date);
    ss >> std::get_time(&tm, "%Y-%m-%d");
    
    auto time_t = std::mktime(&tm);
    auto time_point = std::chrono::system_clock::from_time_t(time_t);
    
    // Add 1 day
    time_point += std::chrono::hours(24);
    
    // If it's Saturday (6) or Sunday (0), move to Monday
    time_t = std::chrono::system_clock::to_time_t(time_point);
    tm = *std::localtime(&time_t);
    
    if (tm.tm_wday == 6) {  // Saturday
        time_point += std::chrono::hours(48);  // Move to Monday
    } else if (tm.tm_wday == 0) {  // Sunday
        time_point += std::chrono::hours(24);  // Move to Monday
    }
    
    time_t = std::chrono::system_clock::to_time_t(time_point);
    tm = *std::localtime(&time_t);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d");
    return oss.str();
}

bool IntegratedMarketPredictionEngine::is_ready() const {
    return (db_manager_ && db_manager_->is_connected() && 
            iqfeed_manager_ && iqfeed_manager_->is_connection_ready());
}

void IntegratedMarketPredictionEngine::print_prediction_summary(const PredictionResult& result) {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "PREDICTION SUMMARY - " << result.symbol << " (" << result.timeframe << ")" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    if (result.prediction_valid) {
        std::cout << "Status: SUCCESS" << std::endl;
        std::cout << "Prediction Date: " << result.prediction_date << std::endl;
        std::cout << "Model: Model 1 Standard (base_alpha=" << result.base_alpha << ")" << std::endl;
        std::cout << "Bars Used: " << result.bars_used << std::endl;
        std::cout << "Confidence: " << std::fixed << std::setprecision(2) 
                  << (result.confidence_score * 100) << "%" << std::endl;
        
        if (result.timeframe == "daily") {
            std::cout << "\nDaily Predictions (Next Business Day):" << std::endl;
            std::cout << "  Open:  " << std::fixed << std::setprecision(6) << result.predicted_open << std::endl;
            std::cout << "  High:  " << std::fixed << std::setprecision(6) << result.predicted_high << std::endl;
            std::cout << "  Low:   " << std::fixed << std::setprecision(6) << result.predicted_low << std::endl;
            std::cout << "  Close: " << std::fixed << std::setprecision(6) << result.predicted_close << std::endl;
        } else {
            std::cout << "\nIntraday Predictions (Next " << result.timeframe << " Interval):" << std::endl;
            std::cout << "  High: " << std::fixed << std::setprecision(6) << result.predicted_next_high << std::endl;
            std::cout << "  Low:  " << std::fixed << std::setprecision(6) << result.predicted_next_low << std::endl;
        }
    } else {
        std::cout << "Status: FAILED" << std::endl;
        std::cout << "Error: " << result.error_message << std::endl;
    }
    
    std::cout << std::string(60, '=') << std::endl;
}

void IntegratedMarketPredictionEngine::handle_prediction_error(const std::string& operation, const std::string& error) {
    logger_->error("Prediction error in " + operation + ": " + error);
}

bool IntegratedMarketPredictionEngine::test_database_connection() {
    return db_manager_ && db_manager_->test_connection();
}

bool IntegratedMarketPredictionEngine::test_iqfeed_connection() {
    return iqfeed_manager_ && iqfeed_manager_->is_connection_ready();
}

void IntegratedMarketPredictionEngine::print_ema_calculation_details(const std::string& symbol) {
    logger_->info("Printing EMA calculation details for: " + symbol);
    
    try {
        // Retrieve historical data for demonstration
        std::vector<PriceBar> historical_data;
        if (!retrieve_historical_data_from_db(symbol, "daily", historical_data)) {
            std::cout << "Failed to retrieve historical data for " << symbol << std::endl;
            std::cout << "Cannot show EMA calculation details without data" << std::endl;
            return;
        }
        
        if (historical_data.size() < MINIMUM_BARS) {
            std::cout << "Insufficient data for EMA calculation" << std::endl;
            std::cout << "Need " << MINIMUM_BARS << " bars, have " << historical_data.size() << std::endl;
            return;
        }
        
        std::cout << "\n" << std::string(80, '=') << std::endl;
        std::cout << "EMA CALCULATION DETAILS - " << symbol << std::endl;
        std::cout << std::string(80, '=') << std::endl;
        
        std::cout << "Model: Model 1 Standard" << std::endl;
        std::cout << "Formula: predicted_t = (base_alpha × current_value) + ((1 - base_alpha) × previous_predict)" << std::endl;
        std::cout << "Base Alpha: " << BASE_ALPHA << std::endl;
        std::cout << "Minimum Bars Required: " << MINIMUM_BARS << std::endl;
        std::cout << "Bootstrap Bars (SMA10): " << BOOTSTRAP_BARS << std::endl;
        
        // Calculate EMA for close prices as example
        EMAResult result = calculate_ema_sequence(historical_data, "close");
        
        if (result.calculation_valid) {
            std::cout << "\nBootstrap SMA10: " << std::fixed << std::setprecision(6) << result.sma10 << std::endl;
            std::cout << "Final EMA Value: " << std::fixed << std::setprecision(6) << result.final_ema << std::endl;
            std::cout << "EMA Sequence Length: " << result.ema_sequence.size() << std::endl;
            
            // Show first few EMA calculations
            std::cout << "\nFirst 5 EMA calculations:" << std::endl;
            for (size_t i = 0; i < std::min(size_t(5), result.ema_sequence.size()); i++) {
                std::cout << "  EMA[" << (i + 1) << "] = " << std::fixed << std::setprecision(6) 
                         << result.ema_sequence[i] << std::endl;
            }
            
            // Show last few EMA calculations
            if (result.ema_sequence.size() > 5) {
                std::cout << "\nLast 3 EMA calculations:" << std::endl;
                size_t start = result.ema_sequence.size() - 3;
                for (size_t i = start; i < result.ema_sequence.size(); i++) {
                    std::cout << "  EMA[" << (i + 1) << "] = " << std::fixed << std::setprecision(6) 
                             << result.ema_sequence[i] << std::endl;
                }
            }
            
        } else {
            std::cout << "\nEMA Calculation FAILED: " << result.error_message << std::endl;
        }
        
        std::cout << std::string(80, '=') << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "Error in EMA calculation details: " << e.what() << std::endl;
    }
}

void IntegratedMarketPredictionEngine::print_system_status() {
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "INTEGRATED PREDICTION ENGINE STATUS" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    std::cout << "Database Connection: " << (test_database_connection() ? "✅ READY" : "❌ FAILED") << std::endl;
    std::cout << "IQFeed Connection: " << (test_iqfeed_connection() ? "✅ READY" : "❌ FAILED") << std::endl;
    std::cout << "Overall Status: " << (is_ready() ? "✅ READY" : "❌ NOT READY") << std::endl;
    std::cout << std::string(50, '=') << std::endl;
}