@echo off
echo === Nexday Database Test Troubleshooting ===
echo.

echo Checking if database_test.exe runs with verbose output...
echo.

REM Try running with pause to see any error messages
echo Running database_test.exe with error capture:
echo ================================================

REM Change to Debug directory and run
cd Debug
database_test.exe
echo.
echo Exit code: %ERRORLEVEL%
echo.

if %ERRORLEVEL% == 0 (
    echo ✓ Program executed successfully but may have had silent errors
) else (
    echo ✗ Program failed with exit code: %ERRORLEVEL%
    echo.
    echo Common issues:
    echo 1. Database connection failed - database may not exist
    echo 2. Missing libpq.dll - PostgreSQL client library not found
    echo 3. Incorrect database credentials
)

echo.
echo Let's check a few things:
echo.

REM Check if PostgreSQL service is running
echo Checking PostgreSQL service status:
sc query postgresql-x64-17
echo.

REM Test basic PostgreSQL connection
echo Testing basic PostgreSQL connection:
set PGPASSWORD=magical.521
"C:\Program Files\PostgreSQL\17\bin\psql" -h localhost -U postgres -c "SELECT 'PostgreSQL is running' AS status;"
echo.

REM Check if nexday_trading database exists
echo Checking if nexday_trading database exists:
"C:\Program Files\PostgreSQL\17\bin\psql" -h localhost -U postgres -l | findstr nexday_trading
echo.

echo ================================================
echo Troubleshooting complete. Check results above.
pause