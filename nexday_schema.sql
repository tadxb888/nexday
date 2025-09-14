-- =========================================================================
-- NEXDAY MARKETS PREDICTIONS SYSTEM - COMPLETE DATABASE SCHEMA
-- Updated for IQFeed Integration with New Historical Data Structure
-- =========================================================================

-- =====================================================
-- 1. REFERENCE TABLES
-- =====================================================

-- Listed Markets
CREATE TABLE IF NOT EXISTS listed_markets (
    market_id INTEGER PRIMARY KEY,
    short_name VARCHAR(20) NOT NULL,
    long_name VARCHAR(100) NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

-- Security Types
CREATE TABLE IF NOT EXISTS security_types (
    security_type_id INTEGER PRIMARY KEY,
    short_name VARCHAR(20) NOT NULL,
    long_name VARCHAR(100) NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

-- =====================================================
-- 2. SYMBOL MANAGEMENT
-- =====================================================

-- Enhanced Symbols Table
CREATE TABLE IF NOT EXISTS symbols (
    symbol_id SERIAL PRIMARY KEY,
    symbol VARCHAR(30) NOT NULL UNIQUE,
    company_name VARCHAR(200),
    security_type_id INTEGER REFERENCES security_types(security_type_id),
    listed_market_id INTEGER REFERENCES listed_markets(market_id),
    exchange_id INTEGER,
    
    -- IQFeed specific fields
    pe_ratio DECIMAL(8,2),
    average_volume BIGINT,
    week_52_high DECIMAL(15,8),
    week_52_low DECIMAL(15,8),
    market_cap BIGINT,
    dividend_yield DECIMAL(5,4),
    eps_current DECIMAL(8,4),
    eps_next_year DECIMAL(8,4),
    
    -- Trading specifications
    tick_size DECIMAL(10,8),
    min_quantity DECIMAL(15,8),
    max_quantity DECIMAL(15,8),
    contract_size VARCHAR(50),
    expiration_date DATE,
    strike_price DECIMAL(15,8),
    
    -- Configuration flags
    is_active BOOLEAN DEFAULT TRUE,
    is_tradeable BOOLEAN DEFAULT TRUE,
    can_short BOOLEAN DEFAULT TRUE,
    
    -- Metadata
    sector VARCHAR(50),
    industry VARCHAR(100),
    country VARCHAR(3) DEFAULT 'USA',
    currency VARCHAR(3) DEFAULT 'USD',
    timezone VARCHAR(50) DEFAULT 'America/New_York',
    metadata JSONB,
    
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

-- =====================================================
-- 3. UPDATED HISTORICAL DATA TABLES FOR IQFEED INTEGRATION
-- =====================================================

-- 3.1 Historical Fetch 15 min
CREATE TABLE IF NOT EXISTS historical_fetch_15min (
    fetch_date          DATE NOT NULL,
    fetch_time          TIME NOT NULL,
    time_of_fetch       TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    symbol_id           INTEGER NOT NULL REFERENCES symbols(symbol_id) ON DELETE CASCADE,
    
    -- Core OHLCV data from IQFeed
    open_price          DECIMAL(15,8) NOT NULL,
    high_price          DECIMAL(15,8) NOT NULL,
    low_price           DECIMAL(15,8) NOT NULL,
    close_price         DECIMAL(15,8) NOT NULL,
    volume              BIGINT NOT NULL,
    open_interest       INTEGER DEFAULT 0,
    
    -- Additional market data (for future calculations)
    vwap                DECIMAL(15,8),
    trade_count         INTEGER,
    bid_price           DECIMAL(15,8),
    ask_price           DECIMAL(15,8),
    spread              DECIMAL(15,8),
    
    -- Technical indicators (calculated - keep for future)
    sma_20              DECIMAL(15,8),
    ema_20              DECIMAL(15,8),
    rsi                 DECIMAL(5,2),
    
    -- Data source and quality tracking
    data_source         VARCHAR(20) DEFAULT 'iqfeed',
    data_quality        VARCHAR(10) DEFAULT 'good',
    is_validated        BOOLEAN DEFAULT FALSE,
    
    PRIMARY KEY (fetch_date, fetch_time, symbol_id)
);

-- 3.2 Historical Fetch 30 min
CREATE TABLE IF NOT EXISTS historical_fetch_30min (
    fetch_date          DATE NOT NULL,
    fetch_time          TIME NOT NULL,
    time_of_fetch       TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    symbol_id           INTEGER NOT NULL REFERENCES symbols(symbol_id) ON DELETE CASCADE,
    
    -- Core OHLCV data from IQFeed
    open_price          DECIMAL(15,8) NOT NULL,
    high_price          DECIMAL(15,8) NOT NULL,
    low_price           DECIMAL(15,8) NOT NULL,
    close_price         DECIMAL(15,8) NOT NULL,
    volume              BIGINT NOT NULL,
    open_interest       INTEGER DEFAULT 0,
    
    -- Additional market data (for future calculations)
    vwap                DECIMAL(15,8),
    trade_count         INTEGER,
    bid_price           DECIMAL(15,8),
    ask_price           DECIMAL(15,8),
    spread              DECIMAL(15,8),
    
    -- Technical indicators (calculated - keep for future)
    sma_20              DECIMAL(15,8),
    ema_20              DECIMAL(15,8),
    rsi                 DECIMAL(5,2),
    
    -- Data source and quality tracking
    data_source         VARCHAR(20) DEFAULT 'iqfeed',
    data_quality        VARCHAR(10) DEFAULT 'good',
    is_validated        BOOLEAN DEFAULT FALSE,
    
    PRIMARY KEY (fetch_date, fetch_time, symbol_id)
);

-- 3.3 Historical Fetch 1 Hour
CREATE TABLE IF NOT EXISTS historical_fetch_1hour (
    fetch_date          DATE NOT NULL,
    fetch_time          TIME NOT NULL,
    time_of_fetch       TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    symbol_id           INTEGER NOT NULL REFERENCES symbols(symbol_id) ON DELETE CASCADE,
    
    -- Core OHLCV data from IQFeed
    open_price          DECIMAL(15,8) NOT NULL,
    high_price          DECIMAL(15,8) NOT NULL,
    low_price           DECIMAL(15,8) NOT NULL,
    close_price         DECIMAL(15,8) NOT NULL,
    volume              BIGINT NOT NULL,
    open_interest       INTEGER DEFAULT 0,
    
    -- Additional market data (for future calculations)
    vwap                DECIMAL(15,8),
    trade_count         INTEGER,
    bid_price           DECIMAL(15,8),
    ask_price           DECIMAL(15,8),
    spread              DECIMAL(15,8),
    
    -- Technical indicators (calculated - keep for future)
    sma_20              DECIMAL(15,8),
    ema_20              DECIMAL(15,8),
    rsi                 DECIMAL(5,2),
    
    -- Data source and quality tracking
    data_source         VARCHAR(20) DEFAULT 'iqfeed',
    data_quality        VARCHAR(10) DEFAULT 'good',
    is_validated        BOOLEAN DEFAULT FALSE,
    
    PRIMARY KEY (fetch_date, fetch_time, symbol_id)
);

-- 3.4 Historical Fetch 2 Hours (NEW - REPLACES 4 HOUR)
CREATE TABLE IF NOT EXISTS historical_fetch_2hours (
    fetch_date          DATE NOT NULL,
    fetch_time          TIME NOT NULL,
    time_of_fetch       TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    symbol_id           INTEGER NOT NULL REFERENCES symbols(symbol_id) ON DELETE CASCADE,
    
    -- Core OHLCV data from IQFeed
    open_price          DECIMAL(15,8) NOT NULL,
    high_price          DECIMAL(15,8) NOT NULL,
    low_price           DECIMAL(15,8) NOT NULL,
    close_price         DECIMAL(15,8) NOT NULL,
    volume              BIGINT NOT NULL,
    open_interest       INTEGER DEFAULT 0,
    
    -- Additional market data (for future calculations)
    vwap                DECIMAL(15,8),
    trade_count         INTEGER,
    bid_price           DECIMAL(15,8),
    ask_price           DECIMAL(15,8),
    spread              DECIMAL(15,8),
    
    -- Technical indicators (calculated - keep for future)
    sma_20              DECIMAL(15,8),
    ema_20              DECIMAL(15,8),
    rsi                 DECIMAL(5,2),
    
    -- Data source and quality tracking
    data_source         VARCHAR(20) DEFAULT 'iqfeed',
    data_quality        VARCHAR(10) DEFAULT 'good',
    is_validated        BOOLEAN DEFAULT FALSE,
    
    PRIMARY KEY (fetch_date, fetch_time, symbol_id)
);

-- 3.5 Historical Fetch Daily
CREATE TABLE IF NOT EXISTS historical_fetch_daily (
    fetch_date          DATE NOT NULL,
    time_of_fetch       TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    symbol_id           INTEGER NOT NULL REFERENCES symbols(symbol_id) ON DELETE CASCADE,
    
    -- Core OHLCV data from IQFeed
    open_price          DECIMAL(15,8) NOT NULL,
    high_price          DECIMAL(15,8) NOT NULL,
    low_price           DECIMAL(15,8) NOT NULL,
    close_price         DECIMAL(15,8) NOT NULL,
    volume              BIGINT NOT NULL,
    open_interest       INTEGER DEFAULT 0,
    
    -- Additional market data (for future calculations)
    vwap                DECIMAL(15,8),
    trade_count         INTEGER,
    adj_close           DECIMAL(15,8),
    bid_price           DECIMAL(15,8),
    ask_price           DECIMAL(15,8),
    spread              DECIMAL(15,8),
    
    -- Technical indicators (calculated - keep for future)
    sma_20              DECIMAL(15,8),
    sma_50              DECIMAL(15,8),
    sma_200             DECIMAL(15,8),
    ema_20              DECIMAL(15,8),
    rsi                 DECIMAL(5,2),
    
    -- Data source and quality tracking
    data_source         VARCHAR(20) DEFAULT 'iqfeed',
    data_quality        VARCHAR(10) DEFAULT 'good',
    is_validated        BOOLEAN DEFAULT FALSE,
    
    PRIMARY KEY (fetch_date, symbol_id)
);

-- =====================================================
-- 4. PREDICTIONS TABLES
-- =====================================================

-- Model Management
CREATE TABLE IF NOT EXISTS model_standard (
    model_id SERIAL PRIMARY KEY,
    model_name VARCHAR(50) NOT NULL UNIQUE,
    model_version VARCHAR(20) NOT NULL,
    timeframe VARCHAR(10) NOT NULL,
    model_type VARCHAR(30) NOT NULL,
    is_active BOOLEAN DEFAULT TRUE,
    is_production_ready BOOLEAN DEFAULT FALSE,
    last_trained TIMESTAMP WITH TIME ZONE,
    next_training_due TIMESTAMP WITH TIME ZONE,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

-- Master predictions table
CREATE TABLE IF NOT EXISTS predictions_all_symbols (
    prediction_id BIGSERIAL PRIMARY KEY,
    prediction_time TIMESTAMP WITH TIME ZONE NOT NULL,
    symbol_id INTEGER NOT NULL REFERENCES symbols(symbol_id) ON DELETE CASCADE,
    model_id INTEGER NOT NULL REFERENCES model_standard(model_id) ON DELETE CASCADE,
    
    -- Prediction data
    timeframe VARCHAR(10) NOT NULL,
    predicted_price DECIMAL(15,8) NOT NULL,
    confidence_score DECIMAL(5,4),
    prediction_horizon INTEGER NOT NULL,
    
    -- Prediction bounds
    price_lower_bound DECIMAL(15,8),
    price_upper_bound DECIMAL(15,8),
    probability_up DECIMAL(5,4),
    probability_down DECIMAL(5,4),
    
    -- Input data
    input_features JSONB,
    current_price DECIMAL(15,8),
    
    -- Validation
    actual_price DECIMAL(15,8),
    prediction_error DECIMAL(15,8),
    prediction_accuracy DECIMAL(5,4),
    is_validated BOOLEAN DEFAULT FALSE,
    validated_at TIMESTAMP WITH TIME ZONE,
    
    -- Metadata
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    expires_at TIMESTAMP WITH TIME ZONE,
    
    UNIQUE(prediction_time, symbol_id, model_id, timeframe)
);

-- =====================================================
-- 5. ERROR TRACKING TABLES (UPDATED FOR 2-HOUR)
-- =====================================================

-- Errors 15 min
CREATE TABLE IF NOT EXISTS errors_15min (
    error_id BIGSERIAL,
    time TIMESTAMP WITH TIME ZONE NOT NULL,
    symbol_id INTEGER REFERENCES symbols(symbol_id),
    error_type VARCHAR(50) NOT NULL,
    error_category VARCHAR(30) NOT NULL,
    error_code VARCHAR(20),
    error_message TEXT NOT NULL,
    stack_trace TEXT,
    severity VARCHAR(10) NOT NULL,
    impact_level VARCHAR(20),
    resolved BOOLEAN DEFAULT FALSE,
    resolution_method VARCHAR(50),
    resolution_notes TEXT,
    resolved_at TIMESTAMP WITH TIME ZONE,
    resolved_by VARCHAR(50),
    occurrence_count INTEGER DEFAULT 1,
    first_occurrence TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    last_occurrence TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    context_data JSONB,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (time, error_id)
);

-- Errors 30 min
CREATE TABLE IF NOT EXISTS errors_30min (
    LIKE errors_15min INCLUDING ALL
);

-- Errors 1 hour
CREATE TABLE IF NOT EXISTS errors_1hour (
    LIKE errors_15min INCLUDING ALL
);

-- Errors 2 hours (REPLACES 4 hour)
CREATE TABLE IF NOT EXISTS errors_2hours (
    LIKE errors_15min INCLUDING ALL
);

-- Errors daily
CREATE TABLE IF NOT EXISTS errors_daily (
    LIKE errors_15min INCLUDING ALL
);

-- =====================================================
-- 6. CLIENTS TABLE
-- =====================================================

CREATE TABLE IF NOT EXISTS clients (
    client_id SERIAL PRIMARY KEY,
    broker_name VARCHAR(100),
    gp_license_id VARCHAR(50),
    status VARCHAR(20) DEFAULT 'active',
    entitlement VARCHAR(50) DEFAULT 'ALL',
    website VARCHAR(255),
    telephone VARCHAR(50),
    email VARCHAR(100),
    country VARCHAR(3),
    ip_address INET,
    private_ip_address INET,
    is_active BOOLEAN DEFAULT TRUE,
    subscription_level VARCHAR(20) DEFAULT 'basic',
    api_key VARCHAR(64) UNIQUE,
    last_login TIMESTAMP WITH TIME ZONE,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

-- =====================================================
-- 7. PERFORMANCE INDEXES
-- =====================================================

-- Symbols
CREATE INDEX IF NOT EXISTS idx_symbols_symbol ON symbols(symbol);
CREATE INDEX IF NOT EXISTS idx_symbols_active ON symbols(is_active) WHERE is_active = TRUE;

-- Historical Data (Time-series optimized with new structure)
CREATE INDEX IF NOT EXISTS idx_historical_15min_symbol_date_time ON historical_fetch_15min(symbol_id, fetch_date DESC, fetch_time DESC);
CREATE INDEX IF NOT EXISTS idx_historical_15min_time_of_fetch ON historical_fetch_15min(time_of_fetch DESC);

CREATE INDEX IF NOT EXISTS idx_historical_30min_symbol_date_time ON historical_fetch_30min(symbol_id, fetch_date DESC, fetch_time DESC);
CREATE INDEX IF NOT EXISTS idx_historical_30min_time_of_fetch ON historical_fetch_30min(time_of_fetch DESC);

CREATE INDEX IF NOT EXISTS idx_historical_1hour_symbol_date_time ON historical_fetch_1hour(symbol_id, fetch_date DESC, fetch_time DESC);
CREATE INDEX IF NOT EXISTS idx_historical_1hour_time_of_fetch ON historical_fetch_1hour(time_of_fetch DESC);

CREATE INDEX IF NOT EXISTS idx_historical_2hours_symbol_date_time ON historical_fetch_2hours(symbol_id, fetch_date DESC, fetch_time DESC);
CREATE INDEX IF NOT EXISTS idx_historical_2hours_time_of_fetch ON historical_fetch_2hours(time_of_fetch DESC);

CREATE INDEX IF NOT EXISTS idx_historical_daily_symbol_date ON historical_fetch_daily(symbol_id, fetch_date DESC);
CREATE INDEX IF NOT EXISTS idx_historical_daily_time_of_fetch ON historical_fetch_daily(time_of_fetch DESC);

-- Predictions
CREATE INDEX IF NOT EXISTS idx_predictions_all_symbols_time ON predictions_all_symbols(prediction_time DESC, symbol_id);
CREATE INDEX IF NOT EXISTS idx_predictions_all_symbols_model ON predictions_all_symbols(model_id, timeframe);

-- Error Management
CREATE INDEX IF NOT EXISTS idx_errors_15min_time_severity ON errors_15min(time DESC, severity, resolved);
CREATE INDEX IF NOT EXISTS idx_errors_30min_time_severity ON errors_30min(time DESC, severity, resolved);
CREATE INDEX IF NOT EXISTS idx_errors_1hour_time_severity ON errors_1hour(time DESC, severity, resolved);
CREATE INDEX IF NOT EXISTS idx_errors_2hours_time_severity ON errors_2hours(time DESC, severity, resolved);
CREATE INDEX IF NOT EXISTS idx_errors_daily_time_severity ON errors_daily(time DESC, severity, resolved);

-- Market Data (legacy compatibility)
CREATE INDEX IF NOT EXISTS idx_market_data_time ON market_data(time DESC);
CREATE INDEX IF NOT EXISTS idx_market_data_symbol_time ON market_data(symbol_id, time DESC);

-- =====================================================
-- 8. SAMPLE DATA
-- =====================================================

-- Insert sample security types
INSERT INTO security_types (security_type_id, short_name, long_name) VALUES
    (1, 'STOCK', 'Common Stock'),
    (2, 'OPTION', 'Option'),
    (3, 'FUTURE', 'Future Contract'),
    (4, 'INDEX', 'Index'),
    (5, 'ETF', 'Exchange Traded Fund'),
    (6, 'FOREX', 'Foreign Exchange'),
    (7, 'CRYPTO', 'Cryptocurrency')
ON CONFLICT (security_type_id) DO NOTHING;

-- Insert sample listed markets
INSERT INTO listed_markets (market_id, short_name, long_name) VALUES
    (1, 'NYSE', 'New York Stock Exchange'),
    (2, 'NASDAQ', 'NASDAQ Stock Market'),
    (3, 'CME', 'Chicago Mercantile Exchange'),
    (4, 'CBOT', 'Chicago Board of Trade'),
    (5, 'NYMEX', 'New York Mercantile Exchange')
ON CONFLICT (market_id) DO NOTHING;

-- Insert sample model
INSERT INTO model_standard (model_name, model_version, timeframe, model_type) VALUES
    ('baseline_v1', '1.0', 'daily', 'regression')
ON CONFLICT (model_name) DO NOTHING;

-- =====================================================
-- 9. DROP OLD 4-HOUR TABLES
-- =====================================================

-- Remove the old 4-hour tables and related objects
DROP TABLE IF EXISTS historical_fetch_4hours CASCADE;
DROP TABLE IF EXISTS errors_4hours CASCADE;
DROP INDEX IF EXISTS idx_historical_4hours_symbol_time;
DROP INDEX IF EXISTS idx_errors_4hours_time_severity;

-- =====================================================
-- 10. COMMENTS FOR DOCUMENTATION
-- =====================================================

COMMENT ON TABLE historical_fetch_15min IS 'IQFeed 15-minute historical OHLCV data with separate date/time columns and fetch timestamp';
COMMENT ON TABLE historical_fetch_30min IS 'IQFeed 30-minute historical OHLCV data with separate date/time columns and fetch timestamp';
COMMENT ON TABLE historical_fetch_1hour IS 'IQFeed 1-hour historical OHLCV data with separate date/time columns and fetch timestamp';
COMMENT ON TABLE historical_fetch_2hours IS 'IQFeed 2-hour historical OHLCV data with separate date/time columns and fetch timestamp';
COMMENT ON TABLE historical_fetch_daily IS 'IQFeed daily historical OHLCV data with date only and fetch timestamp';

COMMENT ON COLUMN historical_fetch_15min.fetch_date IS 'Date of the trading bar (e.g., 2025-09-12)';
COMMENT ON COLUMN historical_fetch_15min.fetch_time IS 'Time of the trading bar (e.g., 07:00:00)';
COMMENT ON COLUMN historical_fetch_15min.time_of_fetch IS 'Timestamp when the data fetch was initiated';
COMMENT ON COLUMN historical_fetch_15min.open_interest IS 'Open interest from IQFeed (futures/options)';

-- =====================================================
-- COMPLETION MESSAGE
-- =====================================================

DO $$ 
BEGIN 
    RAISE NOTICE '';
    RAISE NOTICE '==============================================';
    RAISE NOTICE 'Nexday Markets Predictions System - UPDATED SCHEMA';
    RAISE NOTICE '==============================================';
    RAISE NOTICE '✅ Database schema updated for IQFeed integration!';
    RAISE NOTICE '';
    RAISE NOTICE 'Updated tables:';
    RAISE NOTICE '  • historical_fetch_15min - New date/time structure';
    RAISE NOTICE '  • historical_fetch_30min - New date/time structure';  
    RAISE NOTICE '  • historical_fetch_1hour - New date/time structure';
    RAISE NOTICE '  • historical_fetch_2hours - NEW (replaces 4-hour)';
    RAISE NOTICE '  • historical_fetch_daily - Date only + time_of_fetch';
    RAISE NOTICE '  • errors_2hours - NEW (replaces errors_4hours)';
    RAISE NOTICE '';
    RAISE NOTICE 'Key changes:';
    RAISE NOTICE '  • Added open_interest column to all historical tables';
    RAISE NOTICE '  • Split timestamp into fetch_date + fetch_time for intraday';
    RAISE NOTICE '  • Added time_of_fetch for troubleshooting';
    RAISE NOTICE '  • Updated primary keys for new structure';
    RAISE NOTICE '  • Removed old 4-hour tables';
    RAISE NOTICE '==============================================';
    RAISE NOTICE '';
END $$;