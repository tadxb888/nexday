@echo off
echo === Quick PostgreSQL 17 Connection Test ===
echo.

REM Set PostgreSQL 17 path
set PGPASSWORD=magical.521
set PGPATH="C:\Program Files\PostgreSQL\17\bin"

echo Testing PostgreSQL connection...
%PGPATH%\psql -h localhost -U postgres -c "SELECT version();"

if %ERRORLEVEL% == 0 (
    echo.
    echo ✓ PostgreSQL 17 connection successful!
    echo Ready to create Nexday database.
) else (
    echo ✗ PostgreSQL connection failed!
    echo.
    echo Troubleshooting:
    echo 1. Check if PostgreSQL service is running
    echo 2. Verify password is: magical.521
    echo 3. Make sure PostgreSQL 17 is properly installed
)
echo.
pause