#pragma once

#include <string>
#include <vector>
#include <libpq-fe.h>

// ==============================================
// DATABASE CONFIGURATION
// ==============================================

struct DatabaseConfig {
    std::string host = "localhost";
    int port = 5432;
    std::string database = "nexday_trading";
    std::string username = "nexday_user";
    std::string password = "nexday_secure_password_2025";
    
    std::string to_connection_string() const {
        return "host=" + host + 
               " port=" + std::to_string(port) + 
               " dbname=" + database + 
               " user=" + username + 
               " password=" + password;
    }
};

// ==============================================
// SIMPLE DATABASE MANAGER CLASS (IQFeed Integration Ready)
// ==============================================

class SimpleDatabaseManager {
private:
    DatabaseConfig config_;
    PGconn* connection_;
    bool is_connected_;
    std::string last_error_;
    
    // Private methods
    bool connect_to_database();
    void disconnect_from_database();
    bool execute_query(const std::string& query);
    PGresult* execute_query_with_result(const std::string& query);
    std::string escape_string(const std::string& input);
    int get_symbol_id(const std::string& symbol);
    int get_or_create_symbol_id(const std::string& symbol);
    
public:
    // Constructor and destructor
    explicit SimpleDatabaseManager(const DatabaseConfig& config);
    ~SimpleDatabaseManager();
    
    // Connection management
    bool test_connection();
    bool is_connected() const { return is_connected_; }
    std::string get_last_error() const { return last_error_; }
    
    // ========================================
    // IQFEED HISTORICAL DATA INSERTION METHODS
    // ========================================
    
    // 15-minute data insertion
    bool insert_historical_data_15min(const std::string& symbol, const std::string& date, 
                                      const std::string& time, double open, double high, 
                                      double low, double close, long long volume, int open_interest = 0);
    
    // 30-minute data insertion
    bool insert_historical_data_30min(const std::string& symbol, const std::string& date, 
                                      const std::string& time, double open, double high, 
                                      double low, double close, long long volume, int open_interest = 0);
    
    // 1-hour data insertion
    bool insert_historical_data_1hour(const std::string& symbol, const std::string& date, 
                                      const std::string& time, double open, double high, 
                                      double low, double close, long long volume, int open_interest = 0);
    
    // 2-hour data insertion
    bool insert_historical_data_2hours(const std::string& symbol, const std::string& date, 
                                       const std::string& time, double open, double high, 
                                       double low, double close, long long volume, int open_interest = 0);
    
    // Daily data insertion (no time component)
    bool insert_historical_data_daily(const std::string& symbol, const std::string& date, 
                                      double open, double high, double low, double close, 
                                      long long volume, int open_interest = 0);
    
    // ========================================
    // LEGACY METHODS (for compatibility)
    // ========================================
    
    // Basic data operations (compatible with existing tests)
    bool insert_market_data(const std::string& symbol, double price, long long volume);
    bool insert_historical_data(const std::string& symbol, const std::string& timestamp,
                               double open, double high, double low, double close, long long volume);
    
    // ========================================
    // SYMBOL MANAGEMENT
    // ========================================
    
    std::vector<std::string> get_symbol_list(bool active_only = true);
    bool import_symbols_from_list(const std::vector<std::string>& symbols,
                                 const std::string& import_source = "manual");
    
    // ========================================
    // DEBUG AND MONITORING METHODS
    // ========================================
    
    void print_sample_data();
    void print_table_sizes();
};