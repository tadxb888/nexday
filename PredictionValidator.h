#ifndef PREDICTION_VALIDATOR_H
#define PREDICTION_VALIDATOR_H

#include <string>
#include <vector>
#include <memory>

class SimpleDatabaseManager;
class Logger;

struct ValidationResult {
    int prediction_id = 0;
    std::string timeframe;
    double predicted_price = 0.0;
    double actual_price = 0.0;
    double prediction_error = 0.0;
    double percentage_error = 0.0;
    double accuracy_score = 0.0;
    bool is_valid = false;
    std::string validation_timestamp;
};

struct ModelMetrics {
    int model_id = 0;
    std::string timeframe;
    int total_predictions = 0;
    int validated_predictions = 0;
    double mae = 0.0;
    double rmse = 0.0;
    double mape = 0.0;
    double r_squared = 0.0;
    double mean_accuracy = 0.0;
    double std_deviation = 0.0;
};

class PredictionValidator {
private:
    std::shared_ptr<SimpleDatabaseManager> db_manager_;
    std::unique_ptr<Logger> logger_;
    
    bool update_model_standard_deviation(const ModelMetrics& metrics);
    std::string get_current_timestamp();
    
public:
    explicit PredictionValidator(std::shared_ptr<SimpleDatabaseManager> db_manager);
    ~PredictionValidator(); // Declared here, defined in .cpp where Logger.h is included
    
    bool validate_daily_predictions(const std::string& symbol = "");
    bool validate_intraday_predictions(const std::string& timeframe, const std::string& symbol = "");
    bool validate_all_pending_predictions();
    
    ValidationResult validate_single_prediction(int prediction_id);
    bool update_prediction_validation(const ValidationResult& result);
    
    ModelMetrics calculate_model_metrics(int model_id, const std::string& timeframe, int lookback_days = 30);
    bool update_model_performance(const ModelMetrics& metrics);
    
    std::vector<int> get_unvalidated_predictions(const std::string& timeframe = "");
    double get_actual_price_for_prediction(int prediction_id, const std::string& timeframe,
                                         const std::string& symbol, const std::string& prediction_time);
    
    static double calculate_mae(const std::vector<double>& predicted, const std::vector<double>& actual);
    static double calculate_rmse(const std::vector<double>& predicted, const std::vector<double>& actual);
    static double calculate_mape(const std::vector<double>& predicted, const std::vector<double>& actual);
    static double calculate_r_squared(const std::vector<double>& predicted, const std::vector<double>& actual);
    static double calculate_accuracy_score(double predicted, double actual);
    
    void print_validation_summary(const std::string& timeframe = "");
    void print_model_performance(int model_id);
};

#endif