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
// SIMPLE DATABASE MANAGER CLASS (Compatible with existing code)
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
    
    // Basic data operations (compatible with existing test)
    bool insert_market_data(const std::string& symbol, double price, long long volume);
    bool insert_historical_data(const std::string& symbol, const std::string& timestamp,
                               double open, double high, double low, double close, long long volume);
    
    // Symbol management
    std::vector<std::string> get_symbol_list(bool active_only = true);
    bool import_symbols_from_list(const std::vector<std::string>& symbols,
                                 const std::string& import_source = "manual");
    
    // Debug methods
    void print_sample_data();
    void print_table_sizes();
};