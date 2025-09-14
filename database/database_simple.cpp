#include "database_simple.h"
#include <iostream>
#include <sstream>
#include <iomanip>

// ==============================================
// CONSTRUCTOR AND DESTRUCTOR
// ==============================================

SimpleDatabaseManager::SimpleDatabaseManager(const DatabaseConfig& config)
    : config_(config), connection_(nullptr), is_connected_(false) {
    connect_to_database();
}

SimpleDatabaseManager::~SimpleDatabaseManager() {
    disconnect_from_database();
}

// ==============================================
// CONNECTION MANAGEMENT
// ==============================================

bool SimpleDatabaseManager::connect_to_database() {
    if (is_connected_) {
        return true;
    }
    
    try {
        std::string conn_string = config_.to_connection_string();
        connection_ = PQconnectdb(conn_string.c_str());
        
        if (PQstatus(connection_) != CONNECTION_OK) {
            last_error_ = std::string("Connection failed: ") + PQerrorMessage(connection_);
            std::cerr << last_error_ << std::endl;
            PQfinish(connection_);
            connection_ = nullptr;
            return false;
        }
        
        is_connected_ = true;
        std::cout << "✅ Database connection established successfully" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        last_error_ = std::string("Exception during connection: ") + e.what();
        std::cerr << last_error_ << std::endl;
        return false;
    }
}

void SimpleDatabaseManager::disconnect_from_database() {
    if (connection_) {
        PQfinish(connection_);
        connection_ = nullptr;
    }
    is_connected_ = false;
}

bool SimpleDatabaseManager::test_connection() {
    if (!is_connected_) {
        if (!connect_to_database()) {
            return false;
        }
    }
    
    PGresult* result = PQexec(connection_, "SELECT 1 as test_value");
    ExecStatusType status = PQresultStatus(result);
    
    if (status == PGRES_TUPLES_OK) {
        PQclear(result);
        return true;
    }
    
    last_error_ = std::string("Connection test failed: ") + PQerrorMessage(connection_);
    PQclear(result);
    return false;
}

// ==============================================
// IQFEED HISTORICAL DATA INSERTION METHODS (NEW)
// ==============================================

bool SimpleDatabaseManager::insert_historical_data_15min(const std::string& symbol, const std::string& date, 
                                                         const std::string& time, double open, double high, 
                                                         double low, double close, long long volume, int open_interest) {
    try {
        int symbol_id = get_or_create_symbol_id(symbol);
        if (symbol_id == -1) {
            last_error_ = "Failed to get/create symbol ID for: " + symbol;
            return false;
        }
        
        std::stringstream insert_query;
        insert_query << "INSERT INTO historical_fetch_15min (";
        insert_query << "fetch_date, fetch_time, symbol_id, open_price, high_price, low_price, close_price, volume, open_interest, data_source";
        insert_query << ") VALUES (";
        insert_query << "'" << escape_string(date) << "', ";
        insert_query << "'" << escape_string(time) << "', ";
        insert_query << symbol_id << ", ";
        insert_query << open << ", ";
        insert_query << high << ", ";
        insert_query << low << ", ";
        insert_query << close << ", ";
        insert_query << volume << ", ";
        insert_query << open_interest << ", ";
        insert_query << "'iqfeed'";
        insert_query << ") ON CONFLICT (fetch_date, fetch_time, symbol_id) DO UPDATE SET ";
        insert_query << "open_price = EXCLUDED.open_price, ";
        insert_query << "high_price = EXCLUDED.high_price, ";
        insert_query << "low_price = EXCLUDED.low_price, ";
        insert_query << "close_price = EXCLUDED.close_price, ";
        insert_query << "volume = EXCLUDED.volume, ";
        insert_query << "open_interest = EXCLUDED.open_interest";
        
        return execute_query(insert_query.str());
        
    } catch (const std::exception& e) {
        last_error_ = std::string("Exception in insert_historical_data_15min: ") + e.what();
        std::cerr << last_error_ << std::endl;
        return false;
    }
}

bool SimpleDatabaseManager::insert_historical_data_30min(const std::string& symbol, const std::string& date, 
                                                         const std::string& time, double open, double high, 
                                                         double low, double close, long long volume, int open_interest) {
    try {
        int symbol_id = get_or_create_symbol_id(symbol);
        if (symbol_id == -1) {
            last_error_ = "Failed to get/create symbol ID for: " + symbol;
            return false;
        }
        
        std::stringstream insert_query;
        insert_query << "INSERT INTO historical_fetch_30min (";
        insert_query << "fetch_date, fetch_time, symbol_id, open_price, high_price, low_price, close_price, volume, open_interest, data_source";
        insert_query << ") VALUES (";
        insert_query << "'" << escape_string(date) << "', ";
        insert_query << "'" << escape_string(time) << "', ";
        insert_query << symbol_id << ", ";
        insert_query << open << ", ";
        insert_query << high << ", ";
        insert_query << low << ", ";
        insert_query << close << ", ";
        insert_query << volume << ", ";
        insert_query << open_interest << ", ";
        insert_query << "'iqfeed'";
        insert_query << ") ON CONFLICT (fetch_date, fetch_time, symbol_id) DO UPDATE SET ";
        insert_query << "open_price = EXCLUDED.open_price, ";
        insert_query << "high_price = EXCLUDED.high_price, ";
        insert_query << "low_price = EXCLUDED.low_price, ";
        insert_query << "close_price = EXCLUDED.close_price, ";
        insert_query << "volume = EXCLUDED.volume, ";
        insert_query << "open_interest = EXCLUDED.open_interest";
        
        return execute_query(insert_query.str());
        
    } catch (const std::exception& e) {
        last_error_ = std::string("Exception in insert_historical_data_30min: ") + e.what();
        std::cerr << last_error_ << std::endl;
        return false;
    }
}

bool SimpleDatabaseManager::insert_historical_data_1hour(const std::string& symbol, const std::string& date, 
                                                         const std::string& time, double open, double high, 
                                                         double low, double close, long long volume, int open_interest) {
    try {
        int symbol_id = get_or_create_symbol_id(symbol);
        if (symbol_id == -1) {
            last_error_ = "Failed to get/create symbol ID for: " + symbol;
            return false;
        }
        
        std::stringstream insert_query;
        insert_query << "INSERT INTO historical_fetch_1hour (";
        insert_query << "fetch_date, fetch_time, symbol_id, open_price, high_price, low_price, close_price, volume, open_interest, data_source";
        insert_query << ") VALUES (";
        insert_query << "'" << escape_string(date) << "', ";
        insert_query << "'" << escape_string(time) << "', ";
        insert_query << symbol_id << ", ";
        insert_query << open << ", ";
        insert_query << high << ", ";
        insert_query << low << ", ";
        insert_query << close << ", ";
        insert_query << volume << ", ";
        insert_query << open_interest << ", ";
        insert_query << "'iqfeed'";
        insert_query << ") ON CONFLICT (fetch_date, fetch_time, symbol_id) DO UPDATE SET ";
        insert_query << "open_price = EXCLUDED.open_price, ";
        insert_query << "high_price = EXCLUDED.high_price, ";
        insert_query << "low_price = EXCLUDED.low_price, ";
        insert_query << "close_price = EXCLUDED.close_price, ";
        insert_query << "volume = EXCLUDED.volume, ";
        insert_query << "open_interest = EXCLUDED.open_interest";
        
        return execute_query(insert_query.str());
        
    } catch (const std::exception& e) {
        last_error_ = std::string("Exception in insert_historical_data_1hour: ") + e.what();
        std::cerr << last_error_ << std::endl;
        return false;
    }
}

bool SimpleDatabaseManager::insert_historical_data_2hours(const std::string& symbol, const std::string& date, 
                                                          const std::string& time, double open, double high, 
                                                          double low, double close, long long volume, int open_interest) {
    try {
        int symbol_id = get_or_create_symbol_id(symbol);
        if (symbol_id == -1) {
            last_error_ = "Failed to get/create symbol ID for: " + symbol;
            return false;
        }
        
        std::stringstream insert_query;
        insert_query << "INSERT INTO historical_fetch_2hours (";
        insert_query << "fetch_date, fetch_time, symbol_id, open_price, high_price, low_price, close_price, volume, open_interest, data_source";
        insert_query << ") VALUES (";
        insert_query << "'" << escape_string(date) << "', ";
        insert_query << "'" << escape_string(time) << "', ";
        insert_query << symbol_id << ", ";
        insert_query << open << ", ";
        insert_query << high << ", ";
        insert_query << low << ", ";
        insert_query << close << ", ";
        insert_query << volume << ", ";
        insert_query << open_interest << ", ";
        insert_query << "'iqfeed'";
        insert_query << ") ON CONFLICT (fetch_date, fetch_time, symbol_id) DO UPDATE SET ";
        insert_query << "open_price = EXCLUDED.open_price, ";
        insert_query << "high_price = EXCLUDED.high_price, ";
        insert_query << "low_price = EXCLUDED.low_price, ";
        insert_query << "close_price = EXCLUDED.close_price, ";
        insert_query << "volume = EXCLUDED.volume, ";
        insert_query << "open_interest = EXCLUDED.open_interest";
        
        return execute_query(insert_query.str());
        
    } catch (const std::exception& e) {
        last_error_ = std::string("Exception in insert_historical_data_2hours: ") + e.what();
        std::cerr << last_error_ << std::endl;
        return false;
    }
}

bool SimpleDatabaseManager::insert_historical_data_daily(const std::string& symbol, const std::string& date, 
                                                         double open, double high, double low, double close, 
                                                         long long volume, int open_interest) {
    try {
        int symbol_id = get_or_create_symbol_id(symbol);
        if (symbol_id == -1) {
            last_error_ = "Failed to get/create symbol ID for: " + symbol;
            return false;
        }
        
        std::stringstream insert_query;
        insert_query << "INSERT INTO historical_fetch_daily (";
        insert_query << "fetch_date, symbol_id, open_price, high_price, low_price, close_price, volume, open_interest, data_source";
        insert_query << ") VALUES (";
        insert_query << "'" << escape_string(date) << "', ";
        insert_query << symbol_id << ", ";
        insert_query << open << ", ";
        insert_query << high << ", ";
        insert_query << low << ", ";
        insert_query << close << ", ";
        insert_query << volume << ", ";
        insert_query << open_interest << ", ";
        insert_query << "'iqfeed'";
        insert_query << ") ON CONFLICT (fetch_date, symbol_id) DO UPDATE SET ";
        insert_query << "open_price = EXCLUDED.open_price, ";
        insert_query << "high_price = EXCLUDED.high_price, ";
        insert_query << "low_price = EXCLUDED.low_price, ";
        insert_query << "close_price = EXCLUDED.close_price, ";
        insert_query << "volume = EXCLUDED.volume, ";
        insert_query << "open_interest = EXCLUDED.open_interest";
        
        return execute_query(insert_query.str());
        
    } catch (const std::exception& e) {
        last_error_ = std::string("Exception in insert_historical_data_daily: ") + e.what();
        std::cerr << last_error_ << std::endl;
        return false;
    }
}

// ==============================================
// LEGACY METHODS (kept for compatibility)
// ==============================================

bool SimpleDatabaseManager::insert_market_data(const std::string& symbol, double price, long long volume) {
    try {
        int symbol_id = get_or_create_symbol_id(symbol);
        if (symbol_id == -1) {
            last_error_ = "Failed to get/create symbol ID for: " + symbol;
            return false;
        }
        
        std::stringstream insert_query;
        insert_query << "INSERT INTO market_data (";
        insert_query << "time, symbol_id, last_price, volume, data_source, data_type, created_at";
        insert_query << ") VALUES (";
        insert_query << "CURRENT_TIMESTAMP, ";
        insert_query << symbol_id << ", ";
        insert_query << price << ", ";
        insert_query << volume << ", ";
        insert_query << "'manual', 'historical', ";
        insert_query << "CURRENT_TIMESTAMP";
        insert_query << ")";
        
        return execute_query(insert_query.str());
        
    } catch (const std::exception& e) {
        last_error_ = std::string("Exception in insert_market_data: ") + e.what();
        std::cerr << last_error_ << std::endl;
        return false;
    }
}

bool SimpleDatabaseManager::insert_historical_data(const std::string& symbol, const std::string& timestamp,
                                                  double open, double high, double low, double close, long long volume) {
    // Legacy method - redirect to daily data insertion
    return insert_historical_data_daily(symbol, timestamp, open, high, low, close, volume, 0);
}

// ==============================================
// UTILITY METHODS
// ==============================================

bool SimpleDatabaseManager::execute_query(const std::string& query) {
    if (!is_connected_) {
        last_error_ = "Not connected to database";
        return false;
    }
    
    PGresult* result = PQexec(connection_, query.c_str());
    ExecStatusType status = PQresultStatus(result);
    
    if (status != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK) {
        last_error_ = std::string("Query execution failed: ") + PQerrorMessage(connection_);
        std::cerr << "Query failed: " << last_error_ << std::endl;
        std::cerr << "Query was: " << query << std::endl;
        PQclear(result);
        return false;
    }
    
    PQclear(result);
    return true;
}

PGresult* SimpleDatabaseManager::execute_query_with_result(const std::string& query) {
    if (!is_connected_) {
        last_error_ = "Not connected to database";
        return nullptr;
    }
    
    PGresult* result = PQexec(connection_, query.c_str());
    ExecStatusType status = PQresultStatus(result);
    
    if (status != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK) {
        last_error_ = std::string("Query execution failed: ") + PQerrorMessage(connection_);
        std::cerr << "Query failed: " << last_error_ << std::endl;
        std::cerr << "Query was: " << query << std::endl;
        PQclear(result);
        return nullptr;
    }
    
    return result;
}

std::string SimpleDatabaseManager::escape_string(const std::string& input) {
    if (!connection_) return input;
    
    size_t length = input.length();
    char* escaped = new char[2 * length + 1];
    
    PQescapeStringConn(connection_, escaped, input.c_str(), length, nullptr);
    std::string result(escaped);
    delete[] escaped;
    
    return result;
}

int SimpleDatabaseManager::get_symbol_id(const std::string& symbol) {
    std::string query = "SELECT symbol_id FROM symbols WHERE symbol = '" + escape_string(symbol) + "'";
    PGresult* result = execute_query_with_result(query);
    
    if (!result) {
        return -1;
    }
    
    if (PQntuples(result) == 0) {
        PQclear(result);
        return -1;
    }
    
    int symbol_id = std::stoi(PQgetvalue(result, 0, 0));
    PQclear(result);
    return symbol_id;
}

int SimpleDatabaseManager::get_or_create_symbol_id(const std::string& symbol) {
    // First try to get existing symbol
    int existing_id = get_symbol_id(symbol);
    if (existing_id != -1) {
        return existing_id;
    }
    
    // Create new symbol if it doesn't exist
    std::stringstream insert_query;
    insert_query << "INSERT INTO symbols (symbol, is_active) VALUES ('";
    insert_query << escape_string(symbol) << "', TRUE) RETURNING symbol_id";
    
    PGresult* result = execute_query_with_result(insert_query.str());
    if (!result) {
        return -1;
    }
    
    if (PQntuples(result) == 0) {
        PQclear(result);
        return -1;
    }
    
    int new_symbol_id = std::stoi(PQgetvalue(result, 0, 0));
    PQclear(result);
    
    std::cout << "Created new symbol: " << symbol << " with ID: " << new_symbol_id << std::endl;
    return new_symbol_id;
}

// ==============================================
// SYMBOL MANAGEMENT
// ==============================================

std::vector<std::string> SimpleDatabaseManager::get_symbol_list(bool active_only) {
    std::vector<std::string> result;
    
    try {
        std::string query = "SELECT symbol FROM symbols";
        if (active_only) {
            query += " WHERE is_active = TRUE";
        }
        query += " ORDER BY symbol";
        
        PGresult* pg_result = execute_query_with_result(query);
        if (!pg_result) {
            return result;
        }
        
        int rows = PQntuples(pg_result);
        result.reserve(rows);
        
        for (int i = 0; i < rows; i++) {
            result.push_back(PQgetvalue(pg_result, i, 0));
        }
        
        PQclear(pg_result);
        return result;
        
    } catch (const std::exception& e) {
        last_error_ = std::string("Exception in get_symbol_list: ") + e.what();
        std::cerr << last_error_ << std::endl;
        return result;
    }
}

bool SimpleDatabaseManager::import_symbols_from_list(const std::vector<std::string>& symbols,
                                                   const std::string& import_source) {
    (void)import_source;
    try {
        if (symbols.empty()) {
            last_error_ = "Symbol list is empty";
            return false;
        }
        
        // Start transaction
        if (!execute_query("BEGIN")) {
            return false;
        }
        
        int successful = 0;
        int failed = 0;
        int duplicates = 0;
        
        for (const auto& symbol : symbols) {
            // Check if symbol already exists
            int existing_id = get_symbol_id(symbol);
            if (existing_id != -1) {
                duplicates++;
                continue;
            }
            
            // Insert new symbol
            std::stringstream insert_query;
            insert_query << "INSERT INTO symbols (symbol, is_active, is_tradeable) VALUES ('";
            insert_query << escape_string(symbol) << "', TRUE, TRUE)";
            
            if (execute_query(insert_query.str())) {
                successful++;
                std::cout << "Imported: " << symbol << std::endl;
            } else {
                failed++;
                std::cerr << "Failed to import: " << symbol << std::endl;
            }
        }
        
        // Commit transaction
        if (!execute_query("COMMIT")) {
            execute_query("ROLLBACK");
            return false;
        }
        
        std::cout << "\nImport Summary:" << std::endl;
        std::cout << "  Total symbols: " << symbols.size() << std::endl;
        std::cout << "  Successfully imported: " << successful << std::endl;
        std::cout << "  Failed: " << failed << std::endl;
        std::cout << "  Duplicates (skipped): " << duplicates << std::endl;
        
        if (successful == 0 && failed > 0) {
            last_error_ = "Failed to import any symbols";
            std::cerr << "Failed to import any symbols" << std::endl;
        }
        
        return successful > 0;
        
    } catch (const std::exception& e) {
        execute_query("ROLLBACK");
        last_error_ = std::string("Exception in import_symbols_from_list: ") + e.what();
        std::cerr << last_error_ << std::endl;
        return false;
    }
}

// ==============================================
// DEBUG METHODS
// ==============================================

void SimpleDatabaseManager::print_sample_data() {
    try {
        std::cout << "\n=== SAMPLE DATABASE CONTENTS ===" << std::endl;
        
        // Show symbol count
        std::string symbol_count_query = "SELECT COUNT(*) FROM symbols WHERE is_active = TRUE";
        PGresult* result = execute_query_with_result(symbol_count_query);
        if (result && PQntuples(result) > 0) {
            std::cout << "Active Symbols: " << PQgetvalue(result, 0, 0) << std::endl;
            PQclear(result);
        }
        
        // Show recent historical data from daily table
        std::cout << "\nRecent Daily Historical Data (last 5 records):" << std::endl;
        std::string daily_data_query = 
            "SELECT s.symbol, hd.open_price, hd.high_price, hd.low_price, hd.close_price, hd.volume, hd.fetch_date "
            "FROM historical_fetch_daily hd "
            "JOIN symbols s ON hd.symbol_id = s.symbol_id "
            "ORDER BY hd.time_of_fetch DESC LIMIT 5";
        
        result = execute_query_with_result(daily_data_query);
        if (result) {
            int rows = PQntuples(result);
            if (rows > 0) {
                std::cout << std::setw(10) << "Symbol" << std::setw(10) << "Open" << std::setw(10) << "High" 
                         << std::setw(10) << "Low" << std::setw(10) << "Close" << std::setw(12) << "Volume" 
                         << std::setw(12) << "Date" << std::endl;
                std::cout << std::string(70, '-') << std::endl;
                
                for (int i = 0; i < rows; i++) {
                    std::cout << std::setw(10) << PQgetvalue(result, i, 0)
                             << std::setw(10) << PQgetvalue(result, i, 1)
                             << std::setw(10) << PQgetvalue(result, i, 2)
                             << std::setw(10) << PQgetvalue(result, i, 3)
                             << std::setw(10) << PQgetvalue(result, i, 4)
                             << std::setw(12) << PQgetvalue(result, i, 5)
                             << std::setw(12) << PQgetvalue(result, i, 6) << std::endl;
                }
            } else {
                std::cout << "No historical data found." << std::endl;
            }
            PQclear(result);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Exception in print_sample_data: " << e.what() << std::endl;
    }
}

void SimpleDatabaseManager::print_table_sizes() {
    try {
        std::cout << "\n=== TABLE SIZES ===" << std::endl;
        
        std::vector<std::string> tables = {
            "symbols", "historical_fetch_15min", "historical_fetch_30min", 
            "historical_fetch_1hour", "historical_fetch_2hours", "historical_fetch_daily"
        };
        
        for (const auto& table : tables) {
            std::string query = "SELECT COUNT(*) FROM " + table;
            PGresult* result = execute_query_with_result(query);
            if (result && PQntuples(result) > 0) {
                std::cout << std::setw(25) << table << ": " << PQgetvalue(result, 0, 0) << " rows" << std::endl;
                PQclear(result);
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Exception in print_table_sizes: " << e.what() << std::endl;
    }
}