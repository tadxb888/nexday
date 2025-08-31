#include "database_simple.h"
#include <sstream>
#include <iomanip>

SimpleDatabaseManager::SimpleDatabaseManager(const DatabaseConfig& cfg) 
    : connection(nullptr), connected(false), config(cfg) {
}

SimpleDatabaseManager::~SimpleDatabaseManager() {
    disconnect();
}

bool SimpleDatabaseManager::connect() {
    if (connected) return true;
    
    std::string conn_string = 
        "host=" + config.host + 
        " port=" + std::to_string(config.port) +
        " dbname=" + config.database +
        " user=" + config.username +
        " password=" + config.password;
    
    connection = PQconnectdb(conn_string.c_str());
    
    if (PQstatus(connection) != CONNECTION_OK) {
        std::cout << "Database connection failed: " << PQerrorMessage(connection) << std::endl;
        PQfinish(connection);
        connection = nullptr;
        return false;
    }
    
    connected = true;
    std::cout << "Connected to PostgreSQL database" << std::endl;
    return true;
}

void SimpleDatabaseManager::disconnect() {
    if (connection) {
        PQfinish(connection);
        connection = nullptr;
    }
    connected = false;
}

bool SimpleDatabaseManager::test_connection() {
    if (!connect()) return false;
    
    PGresult* result = PQexec(connection, "SELECT version();");
    
    if (PQresultStatus(result) == PGRES_TUPLES_OK) {
        std::cout << "PostgreSQL version: " << PQgetvalue(result, 0, 0) << std::endl;
        PQclear(result);
        return true;
    } else {
        std::cout << "Test query failed: " << PQerrorMessage(connection) << std::endl;
        PQclear(result);
        return false;
    }
}

int SimpleDatabaseManager::get_symbol_id(const std::string& symbol) {
    if (!connected) return -1;
    
    // Use parameterized query to avoid SQL injection
    const char* paramValues[1] = { symbol.c_str() };
    PGresult* result = PQexecParams(connection,
        "SELECT symbol_id FROM symbols WHERE symbol = $1",
        1,          // number of parameters
        nullptr,    // parameter types (null = infer)
        paramValues,// parameter values
        nullptr,    // parameter lengths (null = text strings)
        nullptr,    // parameter formats (null = text)
        0);         // result format (0 = text)
    
    if (PQresultStatus(result) == PGRES_TUPLES_OK && PQntuples(result) > 0) {
        int symbol_id = std::atoi(PQgetvalue(result, 0, 0));
        PQclear(result);
        return symbol_id;
    }
    
    PQclear(result);
    return -1; // Symbol not found
}

bool SimpleDatabaseManager::insert_market_data(const std::string& symbol, double price, long long volume) {
    if (!connected) return false;
    
    int symbol_id = get_symbol_id(symbol);
    if (symbol_id == -1) {
        std::cout << "Symbol not found: " << symbol << std::endl;
        return false;
    }
    
    // Use parameterized query for safety
    std::string symbol_id_str = std::to_string(symbol_id);
    std::string price_str = std::to_string(price);
    std::string volume_str = std::to_string(volume);
    
    const char* paramValues[4] = { 
        symbol_id_str.c_str(), 
        price_str.c_str(), 
        volume_str.c_str(),
        "streaming" 
    };
    
    PGresult* result = PQexecParams(connection,
        "INSERT INTO market_data (time, symbol_id, last_price, volume, data_type) VALUES (NOW(), $1, $2, $3, $4)",
        4, nullptr, paramValues, nullptr, nullptr, 0);
    
    bool success = (PQresultStatus(result) == PGRES_COMMAND_OK);
    
    if (success) {
        std::cout << "Inserted data for " << symbol << ": $" << std::fixed << std::setprecision(2) << price 
                  << " (Vol: " << volume << ")" << std::endl;
    } else {
        std::cout << "Failed to insert data: " << PQerrorMessage(connection) << std::endl;
    }
    
    PQclear(result);
    return success;
}

bool SimpleDatabaseManager::insert_historical_data(const std::string& symbol, const std::string& timestamp, 
                                                   double open, double high, double low, double close, long long volume) {
    if (!connected) return false;
    
    int symbol_id = get_symbol_id(symbol);
    if (symbol_id == -1) {
        std::cout << "Symbol not found: " << symbol << std::endl;
        return false;
    }
    
    std::string symbol_id_str = std::to_string(symbol_id);
    std::string open_str = std::to_string(open);
    std::string high_str = std::to_string(high);
    std::string low_str = std::to_string(low);
    std::string close_str = std::to_string(close);
    std::string volume_str = std::to_string(volume);
    
    const char* paramValues[7] = { 
        timestamp.c_str(),
        symbol_id_str.c_str(), 
        open_str.c_str(),
        high_str.c_str(),
        low_str.c_str(),
        close_str.c_str(),
        volume_str.c_str()
    };
    
    PGresult* result = PQexecParams(connection,
        "INSERT INTO market_data (time, symbol_id, open_price, high_price, low_price, last_price, volume, data_type) VALUES ($1, $2, $3, $4, $5, $6, $7, 'historical')",
        7, nullptr, paramValues, nullptr, nullptr, 0);
    
    bool success = (PQresultStatus(result) == PGRES_COMMAND_OK);
    
    if (success) {
        std::cout << "Inserted historical data for " << symbol << " at " << timestamp << std::endl;
    } else {
        std::cout << "Failed to insert historical data: " << PQerrorMessage(connection) << std::endl;
    }
    
    PQclear(result);
    return success;
}

bool SimpleDatabaseManager::execute_query(const std::string& query) {
    if (!connected) return false;
    
    PGresult* result = PQexec(connection, query.c_str());
    bool success = (PQresultStatus(result) == PGRES_COMMAND_OK || PQresultStatus(result) == PGRES_TUPLES_OK);
    
    if (!success) {
        std::cout << "Query failed: " << PQerrorMessage(connection) << std::endl;
    }
    
    PQclear(result);
    return success;
}

std::string SimpleDatabaseManager::get_last_error() const {
    if (connection) {
        return std::string(PQerrorMessage(connection));
    }
    return "No connection";
}

void SimpleDatabaseManager::print_sample_data() {
    if (!connected) return;
    
    std::cout << "\n=== Sample Database Data ===" << std::endl;
    
    // Show symbols
    PGresult* result = PQexec(connection, "SELECT symbol, company_name FROM symbols LIMIT 5");
    if (PQresultStatus(result) == PGRES_TUPLES_OK) {
        std::cout << "\nSymbols in database:" << std::endl;
        for (int i = 0; i < PQntuples(result); i++) {
            std::cout << "- " << PQgetvalue(result, i, 0) << ": " << PQgetvalue(result, i, 1) << std::endl;
        }
    }
    PQclear(result);
    
    // Show recent market data
    result = PQexec(connection, 
        "SELECT s.symbol, md.last_price, md.volume, md.time "
        "FROM market_data md JOIN symbols s ON md.symbol_id = s.symbol_id "
        "ORDER BY md.time DESC LIMIT 5");
    
    if (PQresultStatus(result) == PGRES_TUPLES_OK && PQntuples(result) > 0) {
        std::cout << "\nRecent market data:" << std::endl;
        for (int i = 0; i < PQntuples(result); i++) {
            std::cout << "- " << PQgetvalue(result, i, 0) 
                      << ": $" << PQgetvalue(result, i, 1)
                      << " (Vol: " << PQgetvalue(result, i, 2) << ")"
                      << " at " << PQgetvalue(result, i, 3) << std::endl;
        }
    } else {
        std::cout << "\nNo market data found yet." << std::endl;
    }
    PQclear(result);
}