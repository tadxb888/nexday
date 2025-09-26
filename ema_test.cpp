#include "Database/database_simple.h"
#include <iostream>
#include <vector>
#include <algorithm>

// Simple EMA calculation test
class SimpleEMACalculator {
public:
    static double calculate_sma(const std::vector<double>& values, int start_index, int window_size) {
        if (start_index + window_size > values.size()) return 0.0;
        
        double sum = 0.0;
        for (int i = start_index; i < start_index + window_size; i++) {
            sum += values[i];
        }
        return sum / window_size;
    }
    
    static std::vector<double> calculate_ema_sequence(const std::vector<double>& values, double initial_previous_predict) {
        std::vector<double> ema_values;
        double previous_predict = initial_previous_predict;
        const double BASE_ALPHA = 0.5;
        
        for (double current_value : values) {
            double predict_t = (BASE_ALPHA * current_value) + ((1.0 - BASE_ALPHA) * previous_predict);
            ema_values.push_back(predict_t);
            previous_predict = predict_t;
        }
        return ema_values;
    }
};

int main() {
    std::cout << "=== EMA CALCULATION TEST ===" << std::endl;
    
    // Test with sample price data
    std::vector<double> sample_prices = {100.0, 101.0, 102.0, 103.0, 104.0, 103.5, 102.0, 101.5, 102.5, 103.0,
                                        104.0, 105.0, 104.5, 103.0, 102.5, 103.5, 104.0, 105.5, 106.0, 105.0};
    
    std::cout << "Sample prices (" << sample_prices.size() << " bars): ";
    for (size_t i = 0; i < std::min(size_t(10), sample_prices.size()); i++) {
        std::cout << sample_prices[i] << " ";
    }
    std::cout << "..." << std::endl;
    
    // Calculate SMA bootstrap (SMA1-SMA10)
    std::vector<double> sma_values;
    for (int i = 0; i < 10; i++) {
        double sma = SimpleEMACalculator::calculate_sma(sample_prices, i, 5);
        sma_values.push_back(sma);
        std::cout << "SMA" << (i+1) << ": " << sma << std::endl;
    }
    
    // Use SMA10 as initial previous_predict
    double initial_previous_predict = sma_values.back();
    std::cout << "\nUsing SMA10 as initial previous_predict: " << initial_previous_predict << std::endl;
    
    // Calculate EMA sequence from bar 15 onwards
    std::vector<double> ema_input_series(sample_prices.begin() + 14, sample_prices.end());
    auto ema_values = SimpleEMACalculator::calculate_ema_sequence(ema_input_series, initial_previous_predict);
    
    std::cout << "\nEMA sequence:" << std::endl;
    for (size_t i = 0; i < ema_values.size(); i++) {
        std::cout << "EMA" << (i + 15) << ": " << ema_values[i] << std::endl;
    }
    
    double final_prediction = ema_values.back();
    std::cout << "\nFinal EMA prediction: " << final_prediction << std::endl;
    std::cout << "✅ EMA calculation test completed!" << std::endl;
    
    return 0;
}