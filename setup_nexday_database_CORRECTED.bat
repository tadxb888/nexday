@echo off
echo === Complete Nexday Database Setup (PostgreSQL 17) ===
echo.

REM Set PostgreSQL connection (CORRECTED PATH for PostgreSQL 17)
set PGPASSWORD=magical.521
set PGPATH="C:\Program Files\PostgreSQL\17\bin"

echo Using PostgreSQL at: %PGPATH%
echo.

echo Step 1: Creating database and user...
%PGPATH%\psql -h localhost -U postgres -c "CREATE USER nexday_user WITH PASSWORD 'nexday_secure_password_2025';"

echo Creating nexday_trading database...
%PGPATH%\psql -h localhost -U postgres -c "CREATE DATABASE nexday_trading OWNER nexday_user;"

echo Granting permissions...
%PGPATH%\psql -h localhost -U postgres -d nexday_trading -c "GRANT ALL PRIVILEGES ON DATABASE nexday_trading TO nexday_user;"
%PGPATH%\psql -h localhost -U postgres -d nexday_trading -c "GRANT ALL PRIVILEGES ON SCHEMA public TO nexday_user;"

echo.
echo Step 2: Creating tables and schema...
set PGPASSWORD=nexday_secure_password_2025
%PGPATH%\psql -h localhost -U nexday_user -d nexday_trading -f nexday_simple_schema.sql

echo.
echo Step 3: Testing the setup...
%PGPATH%\psql -h localhost -U nexday_user -d nexday_trading -c "SELECT 'Database Ready!' AS status, (SELECT COUNT(*) FROM symbols) AS symbol_count;"

if %ERRORLEVEL% == 0 (
    echo.
    echo ✓ Complete database setup successful!
    echo.
    echo Connection details:
    echo Host: localhost
    echo Port: 5432
    echo Database: nexday_trading
    echo Username: nexday_user
    echo Password: nexday_secure_password_2025
    echo.
    echo Your database is ready for the trading system!
) else (
    echo ✗ Database setup failed!
    echo Check the error messages above.
)
echo.
pause