@echo off
echo === Complete Nexday Database Setup ===
echo.

REM Set PostgreSQL connection
set PGPASSWORD=magical.521
set PGPATH="C:\Program Files\PostgreSQL\17\bin"

echo Step 1: Creating database and user...
%PGPATH%\psql -h localhost -U postgres -c "CREATE USER nexday_user WITH PASSWORD 'nexday_secure_password_2025';"
%PGPATH%\psql -h localhost -U postgres -c "CREATE DATABASE nexday_trading OWNER nexday_user;"
%PGPATH%\psql -h localhost -U postgres -d nexday_trading -c "GRANT ALL PRIVILEGES ON DATABASE nexday_trading TO nexday_user;"
%PGPATH%\psql -h localhost -U postgres -d nexday_trading -c "GRANT ALL PRIVILEGES ON SCHEMA public TO nexday_user;"

echo.
echo Step 2: Creating tables and schema...
set PGPASSWORD=nexday_secure_password_2025
%PGPATH%\psql -h localhost -U nexday_user -d nexday_trading -f nexday_simple_schema.sql

echo.
echo Step 3: Testing the setup...
%PGPATH%\psql -h localhost -U nexday_user -d nexday_trading -c "SELECT 'Database Ready!' AS status;"

echo.
echo ✓ Complete database setup finished!
pause