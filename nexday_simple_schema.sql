-- =========================================================================
-- NEXDAY TRADING SYSTEM - SIMPLE POSTGRESQL SCHEMA
-- =========================================================================
-- Simple PostgreSQL schema without TimescaleDB complexity
-- Ready to use immediately for trading system development
-- =========================================================================

-- =========================================================================
-- 1. REFERENCE TABLES
-- =========================================================================

-- Listed Markets
CREATE TABLE IF NOT EXISTS listed_markets (
    market_id INTEGER PRIMARY KEY,
    short_name VARCHAR(20) NOT NULL,
    long_name VARCHAR(100) NOT NULL,
    created_at TIMESTAMPTZ DEFAULT NOW()
);

-- Security Types
CREATE TABLE IF NOT EXISTS security_types (
    security_type_id INTEGER PRIMARY KEY,
    short_name VARCHAR(20) NOT NULL,
    long_name VARCHAR(100) NOT NULL,
    created_at TIMESTAMPTZ DEFAULT NOW()
);

-- =========================================================================
-- 2. SYMBOLS TABLE
-- =========================================================================

-- Master symbols table
CREATE TABLE IF NOT EXISTS symbols (
    symbol_id SERIAL PRIMARY KEY,
    symbol VARCHAR(50) NOT NULL UNIQUE,
    listed_market_id INTEGER REFERENCES listed_markets(market_id),
    security_type_id INTEGER REFERENCES security_types(security_type_id),
    company_name VARCHAR(255),
    description TEXT,
    is_active BOOLEAN DEFAULT true,
    created_at TIMESTAMPTZ DEFAULT NOW()
);

-- =========================================================================
-- 3. MARKET DATA TABLE (Main Data Storage)
-- =========================================================================

-- Main market data table
CREATE TABLE IF NOT EXISTS market_data (
    id SERIAL PRIMARY KEY,
    time TIMESTAMPTZ NOT NULL,
    symbol_id INTEGER NOT NULL REFERENCES symbols(symbol_id),
    
    -- IQFeed price data
    open_price DECIMAL(15,6),
    high_price DECIMAL(15,6),
    low_price DECIMAL(15,6),
    close_price DECIMAL(15,6),
    last_price DECIMAL(15,6),
    
    -- Volume data
    volume BIGINT,
    total_volume BIGINT,
    
    -- Bid/Ask data
    bid_price DECIMAL(15,6),
    ask_price DECIMAL(15,6),
    bid_size INTEGER,
    ask_size INTEGER,
    
    -- Metadata
    data_source VARCHAR(20) DEFAULT 'iqfeed',
    data_type VARCHAR(20) NOT NULL, -- streaming, historical
    interval_type VARCHAR(10), -- tick, 1min, 5min, etc.
    
    created_at TIMESTAMPTZ DEFAULT NOW()
);

-- =========================================================================
-- 4. PREDICTIONS TABLE (Start Simple)
-- =========================================================================

-- Daily predictions (start with one timeframe)
CREATE TABLE IF NOT EXISTS predictions_daily (
    id SERIAL PRIMARY KEY,
    time TIMESTAMPTZ NOT NULL,
    symbol_id INTEGER NOT NULL REFERENCES symbols(symbol_id),
    predicted_price DECIMAL(15,6) NOT NULL,
    confidence_score DECIMAL(5,4) DEFAULT 0.0,
    prediction_model VARCHAR(50) DEFAULT 'model_v1',
    
    -- Validation (filled when actual data available)
    actual_price DECIMAL(15,6),
    prediction_error DECIMAL(15,6),
    percentage_error DECIMAL(8,4),
    is_validated BOOLEAN DEFAULT false,
    
    created_at TIMESTAMPTZ DEFAULT NOW()
);

-- =========================================================================
-- 5. ERROR TRACKING
-- =========================================================================

-- Simple error logging
CREATE TABLE IF NOT EXISTS error_logs (
    id SERIAL PRIMARY KEY,
    time TIMESTAMPTZ NOT NULL,
    symbol_id INTEGER REFERENCES symbols(symbol_id),
    error_type VARCHAR(50) NOT NULL,
    error_message TEXT,
    component VARCHAR(50), -- iqfeed, database, prediction
    severity VARCHAR(20) DEFAULT 'medium',
    is_resolved BOOLEAN DEFAULT false,
    created_at TIMESTAMPTZ DEFAULT NOW()
);

-- =========================================================================
-- 6. PERFORMANCE INDEXES
-- =========================================================================

-- Critical indexes for trading system
CREATE INDEX IF NOT EXISTS idx_symbols_symbol ON symbols(symbol);
CREATE INDEX IF NOT EXISTS idx_market_data_symbol_time ON market_data (symbol_id, time DESC);
CREATE INDEX IF NOT EXISTS idx_market_data_time ON market_data (time DESC);
CREATE INDEX IF NOT EXISTS idx_predictions_symbol_time ON predictions_daily (symbol_id, time DESC);
CREATE INDEX IF NOT EXISTS idx_error_logs_time ON error_logs (time DESC);

-- =========================================================================
-- 7. SAMPLE DATA FOR TESTING
-- =========================================================================

-- Insert sample markets
INSERT INTO listed_markets (market_id, short_name, long_name) VALUES
(1, 'NASDAQ', 'NASDAQ Stock Market'),
(7, 'NYSE', 'New York Stock Exchange'),
(34, 'CME', 'Chicago Mercantile Exchange')
ON CONFLICT (market_id) DO NOTHING;

-- Insert sample security types
INSERT INTO security_types (security_type_id, short_name, long_name) VALUES
(1, 'EQUITY', 'Common Stock'),
(2, 'INDEX', 'Market Index'),
(11, 'FUTURE', 'Futures Contract')
ON CONFLICT (security_type_id) DO NOTHING;

-- Insert sample symbols for testing
INSERT INTO symbols (symbol, listed_market_id, security_type_id, company_name, description) VALUES
('AAPL', 1, 1, 'Apple Inc.', 'Apple Inc. Common Stock'),
('MSFT', 1, 1, 'Microsoft Corporation', 'Microsoft Corporation'),
('GOOGL', 1, 1, 'Alphabet Inc.', 'Alphabet Inc. Class A'),
('TSLA', 1, 1, 'Tesla Inc.', 'Tesla Inc. Common Stock'),
('SPY', 1, 1, 'SPDR S&P 500 ETF', 'S&P 500 ETF'),
('ES', 34, 11, 'E-mini S&P 500', 'S&P 500 Futures')
ON CONFLICT (symbol) DO NOTHING;

-- =========================================================================
-- 8. USEFUL VIEWS
-- =========================================================================

-- Latest market data view
CREATE OR REPLACE VIEW latest_prices AS
SELECT DISTINCT ON (s.symbol) 
    s.symbol,
    s.symbol_id,
    md.time,
    md.last_price,
    md.volume,
    md.bid_price,
    md.ask_price,
    lm.short_name AS market
FROM market_data md
JOIN symbols s ON md.symbol_id = s.symbol_id
JOIN listed_markets lm ON s.listed_market_id = lm.market_id
ORDER BY s.symbol, md.time DESC;

-- =========================================================================
-- 9. VERIFICATION
-- =========================================================================

-- Verify setup completed successfully
SELECT 
    'PostgreSQL Schema Setup Complete!' AS status,
    (SELECT COUNT(*) FROM symbols) AS symbol_count,
    (SELECT COUNT(*) FROM listed_markets) AS market_count,
    (SELECT COUNT(*) FROM security_types) AS security_type_count;