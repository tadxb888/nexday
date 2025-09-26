#ifndef EMA_CALCULATOR_H
#define EMA_CALCULATOR_H

#include <vector>
#include <iostream>
#include <iomanip>
#include <numeric>
#include <algorithm>

// ==============================================
// SIMPLE EMA CALCULATOR WITH WORKING LOGIC
// ==============================================

class SimpleEMACalculator {
private:
    static constexpr double BASE_ALPHA = 0.5;  // Working value: base_alpha = 0.5
    static constexpr int MIN_BARS_REQUIRED = 15;  // Need 15 bars minimum for SMA10 bootstrap
    
public:
    // Main calculation method - returns prediction for next bar
    static double calculate_prediction(const std::vector<double>& price_data) {
        if (price_data.size() < MIN_BARS_REQUIRED) {
            std::cout << "❌ Insufficient data: " << price_data.size() 
                      << " bars (need " << MIN_BARS_REQUIRED << "+)" << std::endl;
            return 0.0;
        }
        
        // Reverse data so newest is at index 0 (IQFeed format)
        std::vector<double> data = price_data;
        std::reverse(data.begin(), data.end());
        
        // STEP 1: BOOTSTRAP PROCESS - Calculate SMA10 using 5-bar rolling windows
        std::vector<double> sma_values;
        
        // Calculate SMA1 through SMA10 using 5-bar windows
        for (int i = 0; i < 10; i++) {
            double sum = 0.0;
            for (int j = 0; j < 5; j++) {
                sum += data[i + j];
            }
            double sma = sum / 5.0;
            sma_values.push_back(sma);
        }
        
        // SMA10 becomes our initial previous_predict
        double previous_predict = sma_values[9];  // SMA10 is the 10th value (index 9)
        
        // STEP 2: EMA CALCULATION SEQUENCE
        // Start from bar 10 (index 10) since we used bars 0-13 for SMA calculations
        std::vector<double> ema_results;
        
        for (size_t i = 10; i < data.size(); i++) {
            double current_value = data[i];
            
            // EMA formula: predict_t = (base_alpha * current_value) + ((1 - base_alpha) * previous_predict)
            double ema_predict = (BASE_ALPHA * current_value) + ((1 - BASE_ALPHA) * previous_predict);
            
            ema_results.push_back(ema_predict);
            
            // Update previous_predict for next iteration
            previous_predict = ema_predict;
        }
        
        // Return the final EMA result as the prediction for the next bar
        return ema_results.back();
    }
    
    // Debug method to show detailed calculation steps
    static void print_calculation_debug(const std::vector<double>& price_data) {
        if (price_data.size() < MIN_BARS_REQUIRED) {
            std::cout << "❌ Cannot show debug: insufficient data" << std::endl;
            return;
        }
        
        std::cout << "\n🔍 EMA CALCULATION DEBUG:" << std::endl;
        std::cout << "=========================" << std::endl;
        std::cout << "Total bars: " << price_data.size() << std::endl;
        std::cout << "Base Alpha: " << BASE_ALPHA << std::endl;
        std::cout << "Min required bars: " << MIN_BARS_REQUIRED << std::endl;
        
        // Reverse data for calculation
        std::vector<double> data = price_data;
        std::reverse(data.begin(), data.end());
        
        std::cout << "\nSTEP 1: SMA BOOTSTRAP (5-bar rolling windows)" << std::endl;
        std::cout << "--------------------------------------------" << std::endl;
        
        // Show SMA calculations
        std::vector<double> sma_values;
        for (int i = 0; i < 10; i++) {
            double sum = 0.0;
            std::cout << "SMA" << (i+1) << ": bars " << i << "-" << (i+4) << " = ";
            for (int j = 0; j < 5; j++) {
                sum += data[i + j];
                std::cout << std::fixed << std::setprecision(2) << data[i + j];
                if (j < 4) std::cout << " + ";
            }
            double sma = sum / 5.0;
            sma_values.push_back(sma);
            std::cout << " = " << std::fixed << std::setprecision(4) << sma << std::endl;
        }
        
        std::cout << "\nSTEP 2: EMA CALCULATION SEQUENCE" << std::endl;
        std::cout << "--------------------------------" << std::endl;
        std::cout << "Initial previous_predict = SMA10 = " << std::fixed << std::setprecision(4) << sma_values[9] << std::endl;
        
        double previous_predict = sma_values[9];
        std::vector<double> ema_results;
        
        for (size_t i = 10; i < std::min(data.size(), size_t(20)); i++) {  // Show first 10 EMA calculations
            double current_value = data[i];
            double ema_predict = (BASE_ALPHA * current_value) + ((1 - BASE_ALPHA) * previous_predict);
            
            std::cout << "EMA" << (i+1) << ": (" << BASE_ALPHA << " * " << std::fixed << std::setprecision(4) << current_value 
                      << ") + (" << (1-BASE_ALPHA) << " * " << previous_predict << ") = " 
                      << ema_predict << std::endl;
            
            ema_results.push_back(ema_predict);
            previous_predict = ema_predict;
        }
        
        if (data.size() > 20) {
            std::cout << "... (continuing to bar " << data.size() << ")" << std::endl;
            
            // Complete the calculation without printing
            for (size_t i = 20; i < data.size(); i++) {
                double current_value = data[i];
                double ema_predict = (BASE_ALPHA * current_value) + ((1 - BASE_ALPHA) * previous_predict);
                ema_results.push_back(ema_predict);
                previous_predict = ema_predict;
            }
        }
        
        std::cout << "\nFINAL PREDICTION: " << std::fixed << std::setprecision(4) << ema_results.back() << std::endl;
        std::cout << "=================" << std::endl;
    }
    
    // Get the minimum bars required for calculation
    static int get_min_bars_required() {
        return MIN_BARS_REQUIRED;
    }
    
    // Get the base alpha value being used
    static double get_base_alpha() {
        return BASE_ALPHA;
    }
};

#endif // EMA_CALCULATOR_H