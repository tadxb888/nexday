#pragma once

#include <iostream>
#include <string>
#include <memory>

// PostgreSQL 17 header include
#ifdef _WIN32
    #include "C:/Program Files/PostgreSQL/17/include/libpq-fe.h"
#else
    #include <libpq-fe.h>
#endif

/**
 * Database configuration structure
 */
struct DatabaseConfig {
    std::string host = "localhost";
    int port = 5432;
    std::string database = "nexday_trading";
    std::string username = "nexday_user";
    std::string password = "nexday_secure_password_2025";
};

/**
 * Simple database manager for the nexday trading system
 * Handles PostgreSQL connections and basic operations
 */
class SimpleDatabaseManager {
private:
    PGconn* connection;
    bool connected;
    DatabaseConfig config;
    
    /**
     * Get symbol ID from the symbols table
     */
    int get_symbol_id(const std::string& symbol);
    
public:
    /**
     * Constructor
     */
    explicit SimpleDatabaseManager(const DatabaseConfig& cfg);
    
    /**
     * Destructor - ensures clean disconnect
     */
    ~SimpleDatabaseManager();
    
    /**
     * Connect to the database
     */
    bool connect();
    
    /**
     * Disconnect from the database
     */
    void disconnect();
    
    /**
     * Test the database connection and run a simple query
     */
    bool test_connection();
    
    /**
     * Insert market data for a symbol
     */
    bool insert_market_data(const std::string& symbol, double price, long long volume);
    
    /**
     * Insert historical data point
     */
    bool insert_historical_data(const std::string& symbol, const std::string& timestamp, 
                               double open, double high, double low, double close, long long volume);
    
    /**
     * Check if database is connected
     */
    bool is_connected() const { return connected; }
    
    /**
     * Print sample data from the database (for testing)
     */
    void print_sample_data();
    
    /**
     * Execute a raw SQL query (for advanced operations)
     */
    bool execute_query(const std::string& query);
    
    /**
     * Get the last error message
     */
    std::string get_last_error() const;
};