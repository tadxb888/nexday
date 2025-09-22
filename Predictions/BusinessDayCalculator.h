#pragma once

#include <chrono>
#include <ctime>
#include <vector>
#include <sstream>
#include <iomanip>

// ==============================================
// BUSINESS DAY CALCULATOR UTILITY
// ==============================================

class BusinessDayCalculator {
public:
    // Check if a given date is a business day (Monday-Friday)
    static bool is_business_day(const std::chrono::system_clock::time_point& date) {
        auto time_t = std::chrono::system_clock::to_time_t(date);
        auto tm = *std::localtime(&time_t);
        
        // Sunday = 0, Monday = 1, ..., Saturday = 6
        int day_of_week = tm.tm_wday;
        
        // Monday (1) through Friday (5) are business days
        return (day_of_week >= 1 && day_of_week <= 5);
    }
    
    // Get the next business day after the given date
    static std::chrono::system_clock::time_point get_next_business_day(
        const std::chrono::system_clock::time_point& date) {
        
        auto next_day = date + std::chrono::hours(24);
        
        while (!is_business_day(next_day)) {
            next_day += std::chrono::hours(24);
        }
        
        return next_day;
    }
    
    // Get the previous business day before the given date
    static std::chrono::system_clock::time_point get_previous_business_day(
        const std::chrono::system_clock::time_point& date) {
        
        auto prev_day = date - std::chrono::hours(24);
        
        while (!is_business_day(prev_day)) {
            prev_day -= std::chrono::hours(24);
        }
        
        return prev_day;
    }
    
    // Calculate number of business days between two dates
    static int count_business_days_between(
        const std::chrono::system_clock::time_point& start_date,
        const std::chrono::system_clock::time_point& end_date) {
        
        if (start_date >= end_date) {
            return 0;
        }
        
        int count = 0;
        auto current = start_date;
        
        while (current < end_date) {
            if (is_business_day(current)) {
                count++;
            }
            current += std::chrono::hours(24);
        }
        
        return count;
    }
    
    // Get day of week as string
    static std::string get_day_name(const std::chrono::system_clock::time_point& date) {
        auto time_t = std::chrono::system_clock::to_time_t(date);
        auto tm = *std::localtime(&time_t);
        
        const std::vector<std::string> day_names = {
            "Sunday", "Monday", "Tuesday", "Wednesday", 
            "Thursday", "Friday", "Saturday"
        };
        
        return day_names[tm.tm_wday];
    }
    
    // Check if it's Friday (needs Monday prediction)
    static bool is_friday(const std::chrono::system_clock::time_point& date) {
        auto time_t = std::chrono::system_clock::to_time_t(date);
        auto tm = *std::localtime(&time_t);
        return tm.tm_wday == 5; // Friday = 5
    }
    
    // Get date string in YYYY-MM-DD format
    static std::string format_date(const std::chrono::system_clock::time_point& date) {
        auto time_t = std::chrono::system_clock::to_time_t(date);
        auto tm = *std::localtime(&time_t);
        
        char buffer[32];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", &tm);
        return std::string(buffer);
    }
    
    // Get datetime string in ISO format for database
    static std::string format_datetime(const std::chrono::system_clock::time_point& date) {
        auto time_t = std::chrono::system_clock::to_time_t(date);
        auto tm = *std::localtime(&time_t);
        
        char buffer[64];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm);
        return std::string(buffer);
    }
    
    // Parse date string (YYYY-MM-DD format) to time_point
    static std::chrono::system_clock::time_point parse_date(const std::string& date_str) {
        std::tm tm = {};
        std::istringstream ss(date_str);
        ss >> std::get_time(&tm, "%Y-%m-%d");
        
        if (ss.fail()) {
            return std::chrono::system_clock::now(); // Return current time if parsing fails
        }
        
        return std::chrono::system_clock::from_time_t(std::mktime(&tm));
    }
    
    // Get current Eastern Time (market timezone)
    static std::chrono::system_clock::time_point get_current_et() {
        // Note: This is a simplified implementation
        // In production, you'd want to handle EST/EDT properly
        auto utc_now = std::chrono::system_clock::now();
        return utc_now - std::chrono::hours(5); // EST offset (adjust for EDT as needed)
    }
    
    // Check if current time is after market close (assuming 4 PM ET)
    static bool is_after_market_close() {
        auto et_now = get_current_et();
        auto time_t = std::chrono::system_clock::to_time_t(et_now);
        auto tm = *std::localtime(&time_t);
        
        // Market closes at 4 PM (16:00)
        return tm.tm_hour >= 16;
    }
};