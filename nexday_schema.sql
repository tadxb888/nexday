-- =====================================================
-- Nexday Trading System Database Schema
-- PostgreSQL with TimescaleDB-ready design
-- =====================================================

-- Enable TimescaleDB extension (if available)
-- CREATE EXTENSION IF NOT EXISTS timescaledb CASCADE;

-- =====================================================
-- 1. REFERENCE TABLES (Lookup tables for IQFeed data)
-- =====================================================

-- Listed Markets (from IQFeed SLM command)
CREATE TABLE IF NOT EXISTS listed_markets (
    market_id INTEGER PRIMARY KEY,
    short_name VARCHAR(20) NOT NULL,
    long_name VARCHAR(100) NOT NULL,
    group_id INTEGER,
    group_name VARCHAR(50),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Security Types (from IQFeed SST command)
CREATE TABLE IF NOT EXISTS security_types (
    security_type_id INTEGER PRIMARY KEY,
    short_name VARCHAR(20) NOT NULL,
    long_name VARCHAR(100) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Symbols Master Table
CREATE TABLE IF NOT EXISTS symbols (
    symbol_id SERIAL PRIMARY KEY,
    symbol VARCHAR(30) NOT NULL UNIQUE,
    company_name VARCHAR(200),
    security_type_id INTEGER REFERENCES security_types(security_type_id),
    listed_market_id INTEGER REFERENCES listed_markets(market_id),
    exchange_id INTEGER,
    is_active BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- =====================================================
-- 2. CLIENTS TABLE
-- =====================================================

CREATE TABLE IF NOT EXISTS clients (
    client_id SERIAL PRIMARY KEY,
    client_name VARCHAR(100) NOT NULL,
    email VARCHAR(100) UNIQUE,
    api_key VARCHAR(255) UNIQUE,
    subscription_level VARCHAR(20) DEFAULT 'basic',
    is_active BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_login TIMESTAMP
);

-- =====================================================
-- 3. MARKET DATA TABLE (Main time-series data)
-- =====================================================

CREATE TABLE IF NOT EXISTS market_data (
    id BIGSERIAL,
    time TIMESTAMP WITH TIME ZONE NOT NULL,
    symbol_id INTEGER NOT NULL REFERENCES symbols(symbol_id),
    
    -- Price data (from IQFeed F, P, Q messages)
    open_price DECIMAL(12,4),
    high_price DECIMAL(12,4),
    low_price DECIMAL(12,4),
    last_price DECIMAL(12,4) NOT NULL,
    close_price DECIMAL(12,4),
    
    -- Volume and trading data
    volume BIGINT,
    total_volume BIGINT,
    bid_price DECIMAL(12,4),
    ask_price DECIMAL(12,4),
    bid_size INTEGER,
    ask_size INTEGER,
    
    -- Trade information
    trade_size INTEGER,
    trade_conditions VARCHAR(20),
    trade_market_center INTEGER,
    
    -- Data source information
    data_type VARCHAR(20) NOT NULL, -- 'streaming', 'historical', 'fundamental'
    message_type CHAR(1), -- 'F', 'P', 'Q', 'T' from IQFeed
    
    -- Metadata
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    
    PRIMARY KEY (time, symbol_id, id)
);

-- Create hypertable for market_data (TimescaleDB optimization)
-- Uncomment when TimescaleDB is available:
-- SELECT create_hypertable('market_data', 'time', chunk_time_interval => INTERVAL '1 day');

-- =====================================================
-- 4. PREDICTION TABLES (5 different timeframes)
-- =====================================================

-- Daily Predictions
CREATE TABLE IF NOT EXISTS predictions_daily (
    prediction_id BIGSERIAL PRIMARY KEY,
    prediction_time TIMESTAMP WITH TIME ZONE NOT NULL,
    symbol_id INTEGER NOT NULL REFERENCES symbols(symbol_id),
    client_id INTEGER REFERENCES clients(client_id),
    
    -- Prediction data
    predicted_price DECIMAL(12,4) NOT NULL,
    confidence_score DECIMAL(5,4), -- 0.0000 to 1.0000
    prediction_horizon INTEGER DEFAULT 1440, -- minutes (24 hours)
    
    -- Model information
    model_name VARCHAR(50),
    model_version VARCHAR(20),
    
    -- Input features (JSON for flexibility)
    input_features TEXT, -- JSON string of features used
    
    -- Metadata
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    expires_at TIMESTAMP,
    
    UNIQUE(prediction_time, symbol_id, client_id, model_name)
);

-- 15-minute Predictions
CREATE TABLE IF NOT EXISTS predictions_15min (
    prediction_id BIGSERIAL PRIMARY KEY,
    prediction_time TIMESTAMP WITH TIME ZONE NOT NULL,
    symbol_id INTEGER NOT NULL REFERENCES symbols(symbol_id),
    client_id INTEGER REFERENCES clients(client_id),
    
    predicted_price DECIMAL(12,4) NOT NULL,
    confidence_score DECIMAL(5,4),
    prediction_horizon INTEGER DEFAULT 15, -- minutes
    
    model_name VARCHAR(50),
    model_version VARCHAR(20),
    input_features TEXT,
    
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    expires_at TIMESTAMP,
    
    UNIQUE(prediction_time, symbol_id, client_id, model_name)
);

-- 30-minute Predictions
CREATE TABLE IF NOT EXISTS predictions_30min (
    prediction_id BIGSERIAL PRIMARY KEY,
    prediction_time TIMESTAMP WITH TIME ZONE NOT NULL,
    symbol_id INTEGER NOT NULL REFERENCES symbols(symbol_id),
    client_id INTEGER REFERENCES clients(client_id),
    
    predicted_price DECIMAL(12,4) NOT NULL,
    confidence_score DECIMAL(5,4),
    prediction_horizon INTEGER DEFAULT 30, -- minutes
    
    model_name VARCHAR(50),
    model_version VARCHAR(20),
    input_features TEXT,
    
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    expires_at TIMESTAMP,
    
    UNIQUE(prediction_time, symbol_id, client_id, model_name)
);

-- Hourly Predictions
CREATE TABLE IF NOT EXISTS predictions_hourly (
    prediction_id BIGSERIAL PRIMARY KEY,
    prediction_time TIMESTAMP WITH TIME ZONE NOT NULL,
    symbol_id INTEGER NOT NULL REFERENCES symbols(symbol_id),
    client_id INTEGER REFERENCES clients(client_id),
    
    predicted_price DECIMAL(12,4) NOT NULL,
    confidence_score DECIMAL(5,4),
    prediction_horizon INTEGER DEFAULT 60, -- minutes
    
    model_name VARCHAR(50),
    model_version VARCHAR(20),
    input_features TEXT,
    
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    expires_at TIMESTAMP,
    
    UNIQUE(prediction_time, symbol_id, client_id, model_name)
);

-- 4-hour Predictions
CREATE TABLE IF NOT EXISTS predictions_4hour (
    prediction_id BIGSERIAL PRIMARY KEY,
    prediction_time TIMESTAMP WITH TIME ZONE NOT NULL,
    symbol_id INTEGER NOT NULL REFERENCES symbols(symbol_id),
    client_id INTEGER REFERENCES clients(client_id),
    
    predicted_price DECIMAL(12,4) NOT NULL,
    confidence_score DECIMAL(5,4),
    prediction_horizon INTEGER DEFAULT 240, -- minutes
    
    model_name VARCHAR(50),
    model_version VARCHAR(20),
    input_features TEXT,
    
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    expires_at TIMESTAMP,
    
    UNIQUE(prediction_time, symbol_id, client_id, model_name)
);

-- =====================================================
-- 5. ERROR TRACKING TABLES (5 different timeframes)
-- =====================================================

-- Daily Prediction Errors
CREATE TABLE IF NOT EXISTS prediction_errors_daily (
    error_id BIGSERIAL PRIMARY KEY,
    prediction_id BIGINT NOT NULL REFERENCES predictions_daily(prediction_id),
    actual_price DECIMAL(12,4) NOT NULL,
    error_value DECIMAL(12,4) NOT NULL, -- predicted - actual
    absolute_error DECIMAL(12,4) NOT NULL,
    percentage_error DECIMAL(8,4) NOT NULL,
    
    -- Calculated at this time
    calculated_at TIMESTAMP WITH TIME ZONE NOT NULL,
    
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- 15-minute Prediction Errors
CREATE TABLE IF NOT EXISTS prediction_errors_15min (
    error_id BIGSERIAL PRIMARY KEY,
    prediction_id BIGINT NOT NULL REFERENCES predictions_15min(prediction_id),
    actual_price DECIMAL(12,4) NOT NULL,
    error_value DECIMAL(12,4) NOT NULL,
    absolute_error DECIMAL(12,4) NOT NULL,
    percentage_error DECIMAL(8,4) NOT NULL,
    calculated_at TIMESTAMP WITH TIME ZONE NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- 30-minute Prediction Errors
CREATE TABLE IF NOT EXISTS prediction_errors_30min (
    error_id BIGSERIAL PRIMARY KEY,
    prediction_id BIGINT NOT NULL REFERENCES predictions_30min(prediction_id),
    actual_price DECIMAL(12,4) NOT NULL,
    error_value DECIMAL(12,4) NOT NULL,
    absolute_error DECIMAL(12,4) NOT NULL,
    percentage_error DECIMAL(8,4) NOT NULL,
    calculated_at TIMESTAMP WITH TIME ZONE NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Hourly Prediction Errors
CREATE TABLE IF NOT EXISTS prediction_errors_hourly (
    error_id BIGSERIAL PRIMARY KEY,
    prediction_id BIGINT NOT NULL REFERENCES predictions_hourly(prediction_id),
    actual_price DECIMAL(12,4) NOT NULL,
    error_value DECIMAL(12,4) NOT NULL,
    absolute_error DECIMAL(12,4) NOT NULL,
    percentage_error DECIMAL(8,4) NOT NULL,
    calculated_at TIMESTAMP WITH TIME ZONE NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- 4-hour Prediction Errors
CREATE TABLE IF NOT EXISTS prediction_errors_4hour (
    error_id BIGSERIAL PRIMARY KEY,
    prediction_id BIGINT NOT NULL REFERENCES predictions_4hour(prediction_id),
    actual_price DECIMAL(12,4) NOT NULL,
    error_value DECIMAL(12,4) NOT NULL,
    absolute_error DECIMAL(12,4) NOT NULL,
    percentage_error DECIMAL(8,4) NOT NULL,
    calculated_at TIMESTAMP WITH TIME ZONE NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- =====================================================
-- 6. INDEXES FOR PERFORMANCE
-- =====================================================

-- Market Data Indexes
CREATE INDEX IF NOT EXISTS idx_market_data_time ON market_data (time DESC);
CREATE INDEX IF NOT EXISTS idx_market_data_symbol ON market_data (symbol_id);
CREATE INDEX IF NOT EXISTS idx_market_data_symbol_time ON market_data (symbol_id, time DESC);
CREATE INDEX IF NOT EXISTS idx_market_data_data_type ON market_data (data_type);

-- Prediction Indexes
CREATE INDEX IF NOT EXISTS idx_predictions_daily_time ON predictions_daily (prediction_time DESC);
CREATE INDEX IF NOT EXISTS idx_predictions_daily_symbol ON predictions_daily (symbol_id);
CREATE INDEX IF NOT EXISTS idx_predictions_15min_time ON predictions_15min (prediction_time DESC);
CREATE INDEX IF NOT EXISTS idx_predictions_15min_symbol ON predictions_15min (symbol_id);
CREATE INDEX IF NOT EXISTS idx_predictions_30min_time ON predictions_30min (prediction_time DESC);
CREATE INDEX IF NOT EXISTS idx_predictions_30min_symbol ON predictions_30min (symbol_id);
CREATE INDEX IF NOT EXISTS idx_predictions_hourly_time ON predictions_hourly (prediction_time DESC);
CREATE INDEX IF NOT EXISTS idx_predictions_hourly_symbol ON predictions_hourly (symbol_id);
CREATE INDEX IF NOT EXISTS idx_predictions_4hour_time ON predictions_4hour (prediction_time DESC);
CREATE INDEX IF NOT EXISTS idx_predictions_4hour_symbol ON predictions_4hour (symbol_id);

-- Symbol lookup index
CREATE INDEX IF NOT EXISTS idx_symbols_symbol ON symbols (symbol);
CREATE INDEX IF NOT EXISTS idx_symbols_active ON symbols (is_active);

-- =====================================================
-- 7. SAMPLE DATA
-- =====================================================

-- Insert sample security types
INSERT INTO security_types (security_type_id, short_name, long_name) VALUES
    (1, 'STOCK', 'Common Stock'),
    (2, 'OPTION', 'Option'),
    (3, 'FUTURE', 'Future Contract'),
    (4, 'INDEX', 'Index'),
    (5, 'ETF', 'Exchange Traded Fund')
ON CONFLICT (security_type_id) DO NOTHING;

-- Insert sample listed markets
INSERT INTO listed_markets (market_id, short_name, long_name, group_id, group_name) VALUES
    (1, 'NASDAQ', 'NASDAQ National Market', 1, 'NASDAQ'),
    (7, 'NYSE', 'New York Stock Exchange', 7, 'NYSE'),
    (30, 'CBOT', 'Chicago Board Of Trade', 30, 'CBOT'),
    (34, 'CME', 'Chicago Mercantile Exchange', 34, 'CME'),
    (11, 'ARCA', 'NYSE Archipelago', 11, 'NYSE')
ON CONFLICT (market_id) DO NOTHING;

-- Insert sample symbols
INSERT INTO symbols (symbol, company_name, security_type_id, listed_market_id) VALUES
    ('AAPL', 'Apple Inc.', 1, 1),
    ('MSFT', 'Microsoft Corporation', 1, 1),
    ('GOOGL', 'Alphabet Inc.', 1, 1),
    ('TSLA', 'Tesla, Inc.', 1, 1),
    ('SPY', 'SPDR S&P 500 ETF Trust', 5, 11),
    ('QQQ', 'Invesco QQQ Trust', 5, 1),
    ('ES', 'E-mini S&P 500 Future', 3, 34),
    ('NQ', 'E-mini NASDAQ-100 Future', 3, 34),
    ('GC', 'Gold Future', 3, 30),
    ('CL', 'Crude Oil Future', 3, 36)
ON CONFLICT (symbol) DO NOTHING;

-- Insert sample client
INSERT INTO clients (client_name, email, api_key, subscription_level) VALUES
    ('Nexday Trading System', 'admin@nexday.com', 'nexday_api_key_2025', 'premium')
ON CONFLICT (email) DO NOTHING;

-- =====================================================
-- 8. USEFUL VIEWS
-- =====================================================

-- Current market data view (latest price per symbol)
CREATE OR REPLACE VIEW latest_market_data AS
SELECT DISTINCT ON (s.symbol) 
    s.symbol,
    s.company_name,
    md.last_price,
    md.volume,
    md.bid_price,
    md.ask_price,
    md.time,
    md.data_type
FROM market_data md
JOIN symbols s ON md.symbol_id = s.symbol_id
WHERE s.is_active = TRUE
ORDER BY s.symbol, md.time DESC;

-- Prediction accuracy summary view
CREATE OR REPLACE VIEW prediction_accuracy_summary AS
SELECT 
    'daily' as timeframe,
    COUNT(*) as total_predictions,
    AVG(pe.absolute_error) as avg_absolute_error,
    AVG(pe.percentage_error) as avg_percentage_error,
    STDDEV(pe.percentage_error) as std_percentage_error
FROM prediction_errors_daily pe
UNION ALL
SELECT 
    '15min' as timeframe,
    COUNT(*) as total_predictions,
    AVG(pe.absolute_error) as avg_absolute_error,
    AVG(pe.percentage_error) as avg_percentage_error,
    STDDEV(pe.percentage_error) as std_percentage_error
FROM prediction_errors_15min pe
UNION ALL
SELECT 
    '30min' as timeframe,
    COUNT(*) as total_predictions,
    AVG(pe.absolute_error) as avg_absolute_error,
    AVG(pe.percentage_error) as avg_percentage_error,
    STDDEV(pe.percentage_error) as std_percentage_error
FROM prediction_errors_30min pe
UNION ALL
SELECT 
    'hourly' as timeframe,
    COUNT(*) as total_predictions,
    AVG(pe.absolute_error) as avg_absolute_error,
    AVG(pe.percentage_error) as avg_percentage_error,
    STDDEV(pe.percentage_error) as std_percentage_error
FROM prediction_errors_hourly pe
UNION ALL
SELECT 
    '4hour' as timeframe,
    COUNT(*) as total_predictions,
    AVG(pe.absolute_error) as avg_absolute_error,
    AVG(pe.percentage_error) as avg_percentage_error,
    STDDEV(pe.percentage_error) as std_percentage_error
FROM prediction_errors_4hour pe;

-- =====================================================
-- 9. COMPLETION MESSAGE
-- =====================================================

-- Show successful completion
DO $$ 
BEGIN 
    RAISE NOTICE 'Nexday Trading Database Schema Created Successfully!';
    RAISE NOTICE 'Tables created: %, %, %, %, %', 
        'listed_markets', 'security_types', 'symbols', 'clients', 'market_data';
    RAISE NOTICE 'Prediction tables: 5 timeframes (daily, 15min, 30min, hourly, 4hour)';
    RAISE NOTICE 'Error tracking tables: 5 timeframes';
    RAISE NOTICE 'Sample data inserted for testing';
    RAISE NOTICE 'Database is ready for the Nexday Trading System!';
END $$;