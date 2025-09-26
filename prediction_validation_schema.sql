-- =========================================================================
-- NEXDAY MARKETS - PREDICTION VALIDATION SCHEMA ENHANCEMENT
-- =========================================================================
-- Add prediction validation and error calculation tables to existing schema

-- =====================================================
-- 1. ENHANCED PREDICTIONS TABLES
-- =====================================================

-- Drop existing simple predictions table if it exists
DROP TABLE IF EXISTS predictions_daily CASCADE;

-- Enhanced predictions table for all timeframes and OHLC components
CREATE TABLE IF NOT EXISTS predictions_all_symbols (
    prediction_id BIGSERIAL PRIMARY KEY,
    prediction_time TIMESTAMPTZ NOT NULL,
    target_time TIMESTAMPTZ NOT NULL,  -- When this prediction is for
    symbol_id INTEGER NOT NULL REFERENCES symbols(symbol_id),
    model_id INTEGER DEFAULT 1,
    
    -- Prediction components
    timeframe VARCHAR(10) NOT NULL,  -- '15min', '30min', '1hour', '2hour', 'daily'
    prediction_type VARCHAR(20) NOT NULL, -- 'daily_open', 'daily_high', 'daily_low', 'daily_close', '15min_high', '15min_low', etc.
    predicted_value DECIMAL(15,8) NOT NULL,
    confidence_score DECIMAL(5,4) DEFAULT 0.0,
    
    -- Validation data (filled when actual results available)
    actual_value DECIMAL(15,8),
    absolute_error DECIMAL(15,8),
    percentage_error DECIMAL(8,4),
    squared_error DECIMAL(15,8),
    is_validated BOOLEAN DEFAULT FALSE,
    validated_at TIMESTAMPTZ,
    
    -- Model information
    model_name VARCHAR(50) DEFAULT 'Epoch Market Advisor',
    model_parameters JSONB,
    
    created_at TIMESTAMPTZ DEFAULT CURRENT_TIMESTAMP,
    
    UNIQUE(prediction_time, symbol_id, timeframe, prediction_type)
);

-- Daily OHLC predictions table (separate table for easier querying)
CREATE TABLE IF NOT EXISTS predictions_daily (
    prediction_id BIGSERIAL PRIMARY KEY,
    prediction_time TIMESTAMPTZ NOT NULL,
    target_date DATE NOT NULL,  -- Business day this prediction is for
    symbol_id INTEGER NOT NULL REFERENCES symbols(symbol_id),
    model_id INTEGER DEFAULT 1,
    
    -- OHLC predictions
    predicted_open DECIMAL(15,8) NOT NULL,
    predicted_high DECIMAL(15,8) NOT NULL,
    predicted_low DECIMAL(15,8) NOT NULL,
    predicted_close DECIMAL(15,8) NOT NULL,
    confidence_score DECIMAL(5,4) DEFAULT 0.0,
    
    -- Validation data
    actual_open DECIMAL(15,8),
    actual_high DECIMAL(15,8),
    actual_low DECIMAL(15,8),
    actual_close DECIMAL(15,8),
    
    -- Error metrics per component
    open_error DECIMAL(15,8),
    high_error DECIMAL(15,8),
    low_error DECIMAL(15,8),
    close_error DECIMAL(15,8),
    
    open_error_pct DECIMAL(8,4),
    high_error_pct DECIMAL(8,4),
    low_error_pct DECIMAL(8,4),
    close_error_pct DECIMAL(8,4),
    
    is_validated BOOLEAN DEFAULT FALSE,
    validated_at TIMESTAMPTZ,
    
    model_name VARCHAR(50) DEFAULT 'Epoch Market Advisor',
    created_at TIMESTAMPTZ DEFAULT CURRENT_TIMESTAMP,
    
    UNIQUE(target_date, symbol_id, model_id)
);

-- =====================================================
-- 2. PREDICTION ERROR ANALYSIS TABLES
-- =====================================================

-- Error analysis for 15-minute predictions
CREATE TABLE IF NOT EXISTS prediction_errors_15min (
    error_id BIGSERIAL PRIMARY KEY,
    analysis_time TIMESTAMPTZ NOT NULL,
    symbol_id INTEGER NOT NULL REFERENCES symbols(symbol_id),
    symbol_name VARCHAR(50) NOT NULL,
    
    -- Time period analyzed
    period_start TIMESTAMPTZ NOT NULL,
    period_end TIMESTAMPTZ NOT NULL,
    prediction_count INTEGER NOT NULL,
    
    -- High/Low prediction errors
    actual_high DECIMAL(15,8),
    predicted_high DECIMAL(15,8),
    actual_low DECIMAL(15,8),
    predicted_low DECIMAL(15,8),
    
    -- Error Metrics
    mae_high DECIMAL(15,8),  -- Mean Absolute Error for High
    mae_low DECIMAL(15,8),   -- Mean Absolute Error for Low
    rmse_high DECIMAL(15,8), -- Root Mean Square Error for High
    rmse_low DECIMAL(15,8),  -- Root Mean Square Error for Low
    mape_high DECIMAL(8,4),  -- Mean Absolute Percentage Error for High
    mape_low DECIMAL(8,4),   -- Mean Absolute Percentage Error for Low
    smape_high DECIMAL(8,4), -- Symmetric Mean Absolute Percentage Error for High
    smape_low DECIMAL(8,4),  -- Symmetric Mean Absolute Percentage Error for Low
    
    -- Statistical measures
    r_squared_high DECIMAL(8,6), -- R-squared for High predictions
    r_squared_low DECIMAL(8,6),  -- R-squared for Low predictions
    
    -- Directional accuracy
    directional_accuracy_high DECIMAL(5,4), -- % of correct direction predictions
    directional_accuracy_low DECIMAL(5,4),
    matthews_correlation_high DECIMAL(8,6), -- Matthews Correlation Coefficient
    matthews_correlation_low DECIMAL(8,6),
    
    -- Deviation analysis
    max_deviation_high DECIMAL(15,8),
    max_deviation_low DECIMAL(15,8),
    avg_deviation_high DECIMAL(15,8),
    avg_deviation_low DECIMAL(15,8),
    
    created_at TIMESTAMPTZ DEFAULT CURRENT_TIMESTAMP
);

-- Error analysis for 30-minute predictions
CREATE TABLE IF NOT EXISTS prediction_errors_30min (
    LIKE prediction_errors_15min INCLUDING ALL
);

-- Error analysis for 1-hour predictions  
CREATE TABLE IF NOT EXISTS prediction_errors_1hour (
    LIKE prediction_errors_15min INCLUDING ALL
);

-- Error analysis for 2-hour predictions
CREATE TABLE IF NOT EXISTS prediction_errors_2hours (
    LIKE prediction_errors_15min INCLUDING ALL
);

-- Error analysis for daily predictions
CREATE TABLE IF NOT EXISTS prediction_errors_daily (
    error_id BIGSERIAL PRIMARY KEY,
    analysis_time TIMESTAMPTZ NOT NULL,
    symbol_id INTEGER NOT NULL REFERENCES symbols(symbol_id),
    symbol_name VARCHAR(50) NOT NULL,
    
    -- Time period analyzed
    period_start DATE NOT NULL,
    period_end DATE NOT NULL,
    prediction_count INTEGER NOT NULL,
    
    -- OHLC actual vs predicted
    actual_open DECIMAL(15,8),
    predicted_open DECIMAL(15,8),
    actual_high DECIMAL(15,8),
    predicted_high DECIMAL(15,8),
    actual_low DECIMAL(15,8),
    predicted_low DECIMAL(15,8),
    actual_close DECIMAL(15,8),
    predicted_close DECIMAL(15,8),
    
    -- Error Metrics for each OHLC component
    mae_open DECIMAL(15,8),
    mae_high DECIMAL(15,8),
    mae_low DECIMAL(15,8),
    mae_close DECIMAL(15,8),
    
    rmse_open DECIMAL(15,8),
    rmse_high DECIMAL(15,8),
    rmse_low DECIMAL(15,8),
    rmse_close DECIMAL(15,8),
    
    mape_open DECIMAL(8,4),
    mape_high DECIMAL(8,4),
    mape_low DECIMAL(8,4),
    mape_close DECIMAL(8,4),
    
    smape_open DECIMAL(8,4),
    smape_high DECIMAL(8,4),
    smape_low DECIMAL(8,4),
    smape_close DECIMAL(8,4),
    
    -- R-squared for each component
    r_squared_open DECIMAL(8,6),
    r_squared_high DECIMAL(8,6),
    r_squared_low DECIMAL(8,6),
    r_squared_close DECIMAL(8,6),
    
    -- Directional accuracy
    directional_accuracy_open DECIMAL(5,4),
    directional_accuracy_high DECIMAL(5,4),
    directional_accuracy_low DECIMAL(5,4),
    directional_accuracy_close DECIMAL(5,4),
    
    -- Matthews Correlation Coefficient
    matthews_correlation_open DECIMAL(8,6),
    matthews_correlation_high DECIMAL(8,6),
    matthews_correlation_low DECIMAL(8,6),
    matthews_correlation_close DECIMAL(8,6),
    
    -- Overall model performance
    overall_mae DECIMAL(15,8),
    overall_rmse DECIMAL(15,8),
    overall_mape DECIMAL(8,4),
    overall_accuracy DECIMAL(5,4),
    
    created_at TIMESTAMPTZ DEFAULT CURRENT_TIMESTAMP
);

-- =====================================================
-- 3. PERFORMANCE INDEXES
-- =====================================================

-- Predictions indexes
CREATE INDEX IF NOT EXISTS idx_predictions_all_symbols_time_symbol ON predictions_all_symbols(prediction_time DESC, symbol_id);
CREATE INDEX IF NOT EXISTS idx_predictions_all_symbols_target_time ON predictions_all_symbols(target_time DESC);
CREATE INDEX IF NOT EXISTS idx_predictions_all_symbols_validation ON predictions_all_symbols(is_validated, validated_at DESC);
CREATE INDEX IF NOT EXISTS idx_predictions_all_symbols_type ON predictions_all_symbols(prediction_type, timeframe);

CREATE INDEX IF NOT EXISTS idx_predictions_daily_target_symbol ON predictions_daily(target_date DESC, symbol_id);
CREATE INDEX IF NOT EXISTS idx_predictions_daily_validation ON predictions_daily(is_validated, validated_at DESC);

-- Error analysis indexes
CREATE INDEX IF NOT EXISTS idx_prediction_errors_15min_time ON prediction_errors_15min(analysis_time DESC);
CREATE INDEX IF NOT EXISTS idx_prediction_errors_15min_symbol ON prediction_errors_15min(symbol_id, analysis_time DESC);

CREATE INDEX IF NOT EXISTS idx_prediction_errors_30min_time ON prediction_errors_30min(analysis_time DESC);
CREATE INDEX IF NOT EXISTS idx_prediction_errors_30min_symbol ON prediction_errors_30min(symbol_id, analysis_time DESC);

CREATE INDEX IF NOT EXISTS idx_prediction_errors_1hour_time ON prediction_errors_1hour(analysis_time DESC);
CREATE INDEX IF NOT EXISTS idx_prediction_errors_1hour_symbol ON prediction_errors_1hour(symbol_id, analysis_time DESC);

CREATE INDEX IF NOT EXISTS idx_prediction_errors_2hours_time ON prediction_errors_2hours(analysis_time DESC);
CREATE INDEX IF NOT EXISTS idx_prediction_errors_2hours_symbol ON prediction_errors_2hours(symbol_id, analysis_time DESC);

CREATE INDEX IF NOT EXISTS idx_prediction_errors_daily_time ON prediction_errors_daily(analysis_time DESC);
CREATE INDEX IF NOT EXISTS idx_prediction_errors_daily_symbol ON prediction_errors_daily(symbol_id, analysis_time DESC);

-- =====================================================
-- 4. USEFUL VIEWS FOR MONITORING
-- =====================================================

-- Current prediction accuracy by timeframe
CREATE OR REPLACE VIEW prediction_accuracy_summary AS
SELECT 
    pas.timeframe,
    COUNT(*) as total_predictions,
    COUNT(*) FILTER (WHERE pas.is_validated = TRUE) as validated_predictions,
    AVG(ABS(pas.percentage_error)) FILTER (WHERE pas.is_validated = TRUE) as avg_absolute_percentage_error,
    STDDEV(pas.percentage_error) FILTER (WHERE pas.is_validated = TRUE) as error_std_dev,
    AVG(pas.confidence_score) as avg_confidence,
    MIN(pas.prediction_time) as first_prediction,
    MAX(pas.prediction_time) as latest_prediction
FROM predictions_all_symbols pas
GROUP BY pas.timeframe
ORDER BY pas.timeframe;

-- Daily prediction performance
CREATE OR REPLACE VIEW daily_prediction_performance AS
SELECT 
    s.symbol,
    pd.target_date,
    pd.predicted_open,
    pd.actual_open,
    pd.open_error_pct,
    pd.predicted_high,
    pd.actual_high,
    pd.high_error_pct,
    pd.predicted_low,
    pd.actual_low,
    pd.low_error_pct,
    pd.predicted_close,
    pd.actual_close,
    pd.close_error_pct,
    pd.confidence_score,
    pd.is_validated,
    CASE 
        WHEN pd.is_validated THEN 
            (ABS(pd.open_error_pct) + ABS(pd.high_error_pct) + ABS(pd.low_error_pct) + ABS(pd.close_error_pct)) / 4.0
        ELSE NULL 
    END as avg_error_pct
FROM predictions_daily pd
JOIN symbols s ON pd.symbol_id = s.symbol_id
ORDER BY pd.target_date DESC, s.symbol;

-- Model performance comparison
CREATE OR REPLACE VIEW model_performance_comparison AS
SELECT 
    pas.model_name,
    pas.timeframe,
    COUNT(*) as total_predictions,
    COUNT(*) FILTER (WHERE pas.is_validated = TRUE) as validated_count,
    ROUND(AVG(ABS(pas.percentage_error)) FILTER (WHERE pas.is_validated = TRUE), 4) as avg_abs_error_pct,
    ROUND(AVG(pas.confidence_score), 4) as avg_confidence,
    ROUND(
        COUNT(*) FILTER (WHERE pas.is_validated = TRUE AND ABS(pas.percentage_error) < 1.0) * 100.0 / 
        NULLIF(COUNT(*) FILTER (WHERE pas.is_validated = TRUE), 0), 2
    ) as accuracy_within_1pct,
    ROUND(
        COUNT(*) FILTER (WHERE pas.is_validated = TRUE AND ABS(pas.percentage_error) < 2.0) * 100.0 / 
        NULLIF(COUNT(*) FILTER (WHERE pas.is_validated = TRUE), 0), 2
    ) as accuracy_within_2pct
FROM predictions_all_symbols pas
GROUP BY pas.model_name, pas.timeframe
ORDER BY avg_abs_error_pct ASC NULLS LAST;

-- =====================================================
-- 5. SAMPLE MODEL RECORD
-- =====================================================

-- Ensure model exists for prediction system
INSERT INTO model_standard (model_name, model_version, timeframe, model_type, is_active, is_production_ready) VALUES
    ('Epoch Market Advisor', '1.0', 'multi', 'technical_analysis', TRUE, TRUE)
ON CONFLICT DO NOTHING;

-- =====================================================
-- COMPLETION MESSAGE
-- =====================================================

DO $$ 
BEGIN 
    RAISE NOTICE '';
    RAISE NOTICE '==============================================';
    RAISE NOTICE 'NEXDAY MARKETS - PREDICTION VALIDATION SCHEMA';
    RAISE NOTICE '==============================================';
    RAISE NOTICE '✅ Enhanced prediction tables created!';
    RAISE NOTICE '';
    RAISE NOTICE 'New/Updated Tables:';
    RAISE NOTICE '  • predictions_all_symbols - All predictions with validation';
    RAISE NOTICE '  • predictions_daily - Daily OHLC predictions';  
    RAISE NOTICE '  • prediction_errors_15min - 15min error analysis';
    RAISE NOTICE '  • prediction_errors_30min - 30min error analysis';
    RAISE NOTICE '  • prediction_errors_1hour - 1hour error analysis';
    RAISE NOTICE '  • prediction_errors_2hours - 2hour error analysis';
    RAISE NOTICE '  • prediction_errors_daily - Daily error analysis';
    RAISE NOTICE '';
    RAISE NOTICE 'Error Metrics Included:';
    RAISE NOTICE '  • MAE (Mean Absolute Error)';
    RAISE NOTICE '  • RMSE (Root Mean Square Error)';
    RAISE NOTICE '  • MAPE (Mean Absolute Percentage Error)';
    RAISE NOTICE '  • SMAPE (Symmetric Mean Absolute Percentage Error)';
    RAISE NOTICE '  • R² (Coefficient of Determination)';
    RAISE NOTICE '  • Matthews Correlation Coefficient';
    RAISE NOTICE '  • Directional Accuracy';
    RAISE NOTICE '';
    RAISE NOTICE 'Views Created:';
    RAISE NOTICE '  • prediction_accuracy_summary';
    RAISE NOTICE '  • daily_prediction_performance';
    RAISE NOTICE '  • model_performance_comparison';
    RAISE NOTICE '';
    RAISE NOTICE '✅ Ready for prediction validation system!';
    RAISE NOTICE '==============================================';
    RAISE NOTICE '';
END $$;