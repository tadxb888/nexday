@echo off
echo === Database Verification Script ===
echo.

REM Set PostgreSQL 17 connection
set PGPASSWORD=magical.521
set PGPATH="C:\Program Files\PostgreSQL\17\bin"

echo Testing basic PostgreSQL connection...
%PGPATH%\psql -h localhost -U postgres -c "SELECT version();"

echo.
echo Checking if nexday_user exists...
%PGPATH%\psql -h localhost -U postgres -c "SELECT usename FROM pg_user WHERE usename = 'nexday_user';"

echo.
echo Checking if nexday_trading database exists...
%PGPATH%\psql -h localhost -U postgres -c "SELECT datname FROM pg_database WHERE datname = 'nexday_trading';"

echo.
echo Testing nexday_user connection...
set PGPASSWORD=nexday_secure_password_2025
%PGPATH%\psql -h localhost -U nexday_user -d nexday_trading -c "SELECT 'User connection works!' AS test;"

echo.
echo Checking if tables exist...
%PGPATH%\psql -h localhost -U nexday_user -d nexday_trading -c "SELECT table_name FROM information_schema.tables WHERE table_schema = 'public';"

echo.
echo Checking symbols data...
%PGPATH%\psql -h localhost -U nexday_user -d nexday_trading -c "SELECT symbol, company_name FROM symbols;"

echo.
echo Verification complete!
pause