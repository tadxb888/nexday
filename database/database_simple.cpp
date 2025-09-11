#include "database_simple.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>

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
    try {
        std::string conn_str = config_.to_connection_string();
        connection_ = PQconnectdb(conn_str.c_str());
        
        if (PQstatus(connection_) != CONNECTION_OK) {
            last_error_ = std::string("Connection to database failed: ") + PQerrorMessage(connection_);
            std::cerr << "Database connection error: " << last_error_ << std::endl;
            PQfinish(connection_);
            connection_ = nullptr;
            is_connected_ = false;
            return false;
        }
        
        is_connected_ = true;
        std::cout << "Successfully connected to PostgreSQL database" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        last_error_ = std::string("Exception during database connection: ") + e.what();
        std::cerr << "Database connection exception: " << last_error_ << std::endl;
        is_connected_ = false;
        return false;
    }
}

void SimpleDatabaseManager::disconnect_from_database() {
    if (connection_) {
        PQfinish(connection_);
        connection_ = nullptr;
        is_connected_ = false;
        std::cout << "Database connection closed" << std::endl;
    }
}

bool SimpleDatabaseManager::test_connection() {
    if (!is_connected_ || PQstatus(connection_) != CONNECTION_OK) {
        last_error_ = "Not connected to database";
        return false;
    }
    
    PGresult* result = PQexec(connection_, "SELECT 1");
    if (PQresultStatus(result) == PGRES_TUPLES_OK) {
        PQclear(result);
        return true;
    }
    
    last_error_ = std::string("Connection test failed: ") + PQerrorMessage(connection_);
    PQclear(result);
    return false;
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
    
    if (!result || PQntuples(result) == 0) {
        if (result) PQclear(result);
        return -1;
    }
    
    int symbol_id = std::atoi(PQgetvalue(result, 0, 0));
    PQclear(result);
    return symbol_id;
}

int SimpleDatabaseManager::get_or_create_symbol_id(const std::string& symbol) {
    int existing_id = get_symbol_id(symbol);
    if (existing_id != -1) {
        return existing_id;
    }
    
    // Create new symbol
    std::string insert_query = 
        "INSERT INTO symbols (symbol, is_active, created_at) "
        "VALUES ('" + escape_string(symbol) + "', TRUE, CURRENT_TIMESTAMP) "
        "RETURNING symbol_id";
    
    PGresult* result = execute_query_with_result(insert_query);
    if (!result || PQntuples(result) == 0) {
        if (result) PQclear(result);
        return -1;
    }
    
    int new_id = std::atoi(PQgetvalue(result, 0, 0));
    PQclear(result);
    return new_id;
}

// ==============================================
// DATA OPERATIONS
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
        insert_query << "time, symbol_id, last_price, volume, message_type, data_type, created_at";
        insert_query << ") VALUES (";
        insert_query << "CURRENT_TIMESTAMP, ";
        insert_query << symbol_id << ", ";
        insert_query << price << ", ";
        insert_query << volume << ", ";
        insert_query << "'P', ";  // Summary message
        insert_query << "'manual', ";
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
    try {
        int symbol_id = get_or_create_symbol_id(symbol);
        if (symbol_id == -1) {
            last_error_ = "Failed to get/create symbol ID for: " + symbol;
            return false;
        }
        
        std::stringstream insert_query;
        insert_query << "INSERT INTO historical_fetch_daily (";
        insert_query << "time, symbol_id, open_price, high_price, low_price, close_price, volume, data_source";
        insert_query << ") VALUES (";
        insert_query << "'" << escape_string(timestamp) << "', ";
        insert_query << symbol_id << ", ";
        insert_query << open << ", ";
        insert_query << high << ", ";
        insert_query << low << ", ";
        insert_query << close << ", ";
        insert_query << volume << ", ";
        insert_query << "'manual'";
        insert_query << ") ON CONFLICT (time, symbol_id) DO UPDATE SET ";
        insert_query << "open_price = EXCLUDED.open_price, ";
        insert_query << "close_price = EXCLUDED.close_price";
        
        return execute_query(insert_query.str());
        
    } catch (const std::exception& e) {
        last_error_ = std::string("Exception in insert_historical_data: ") + e.what();
        std::cerr << last_error_ << std::endl;
        return false;
    }
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
        
    } catch (const std::exception& e) {
        last_error_ = std::string("Exception in get_symbol_list: ") + e.what();
        std::cerr << last_error_ << std::endl;
    }
    
    return result;
}

bool SimpleDatabaseManager::import_symbols_from_list(const std::vector<std::string>& symbols,
                                                   const std::string& import_source) {
    if (symbols.empty()) {
        return true;
    }
    
    try {
        // Begin transaction
        if (!execute_query("BEGIN")) {
            return false;
        }
        
        int successful = 0;
        int failed = 0;
        
        for (const std::string& symbol : symbols) {
            // Simple validation
            if (symbol.empty() || symbol.length() > 30) {
                failed++;
                continue;
            }
            
            // Check if symbol already exists
            int existing_id = get_symbol_id(symbol);
            if (existing_id != -1) {
                // Update existing symbol to be active
                std::string update_query = 
                    "UPDATE symbols SET is_active = TRUE, updated_at = CURRENT_TIMESTAMP "
                    "WHERE symbol = '" + escape_string(symbol) + "'";
                
                if (execute_query(update_query)) {
                    successful++;
                } else {
                    failed++;
                }
            } else {
                // Insert new symbol
                std::string insert_query = 
                    "INSERT INTO symbols (symbol, is_active, created_at) "
                    "VALUES ('" + escape_string(symbol) + "', TRUE, CURRENT_TIMESTAMP)";
                
                if (execute_query(insert_query)) {
                    successful++;
                } else {
                    failed++;
                }
            }
        }
        
        if (successful > 0) {
            execute_query("COMMIT");
            std::cout << "Successfully imported " << successful << "/" << symbols.size() 
                     << " symbols" << std::endl;
        } else {
            execute_query("ROLLBACK");
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
        
        // Show recent market data
        std::cout << "\nRecent Market Data (last 5 records):" << std::endl;
        std::string market_data_query = 
            "SELECT s.symbol, md.last_price, md.volume, md.time "
            "FROM market_data md "
            "JOIN symbols s ON md.symbol_id = s.symbol_id "
            "ORDER BY md.time DESC LIMIT 5";
        
        result = execute_query_with_result(market_data_query);
        if (result) {
            int rows = PQntuples(result);
            if (rows > 0) {
                std::cout << std::setw(10) << "Symbol" << std::setw(12) << "Price" 
                         << std::setw(15) << "Volume" << std::setw(22) << "Timestamp" << std::endl;
                std::cout << std::string(59, '-') << std::endl;
                
                for (int i = 0; i < rows; i++) {
                    std::cout << std::setw(10) << PQgetvalue(result, i, 0)
                             << std::setw(12) << PQgetvalue(result, i, 1)
                             << std::setw(15) << PQgetvalue(result, i, 2)
                             << std::setw(22) << PQgetvalue(result, i, 3) << std::endl;
                }
            } else {
                std::cout << "No market data found." << std::endl;
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
            "symbols", "market_data", "clients", "listed_markets", "security_types"
        };
        
        std::cout << std::setw(25) << "Table Name" << std::setw(15) << "Row Count" << std::endl;
        std::cout << std::string(40, '-') << std::endl;
        
        for (const std::string& table : tables) {
            std::string query = "SELECT COUNT(*) FROM " + table + " WHERE 1=1"; // Safe query
            PGresult* result = execute_query_with_result(query);
            if (result && PQntuples(result) > 0) {
                std::cout << std::setw(25) << table 
                         << std::setw(15) << PQgetvalue(result, 0, 0) << std::endl;
                PQclear(result);
            } else {
                std::cout << std::setw(25) << table 
                         << std::setw(15) << "N/A" << std::endl;
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Exception in print_table_sizes: " << e.what() << std::endl;
    }
}