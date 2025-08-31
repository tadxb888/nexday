@echo off
echo ====================================================
echo PostgreSQL 17 File Verification for CMake
echo ====================================================
echo.

echo Checking required PostgreSQL 17 files...
echo.

REM Check include directory
if exist "C:\Program Files\PostgreSQL\17\include\libpq-fe.h" (
    echo ✓ FOUND: C:\Program Files\PostgreSQL\17\include\libpq-fe.h
) else (
    echo ❌ MISSING: C:\Program Files\PostgreSQL\17\include\libpq-fe.h
    set MISSING=1
)

REM Check library file
if exist "C:\Program Files\PostgreSQL\17\lib\libpq.lib" (
    echo ✓ FOUND: C:\Program Files\PostgreSQL\17\lib\libpq.lib
) else (
    echo ❌ MISSING: C:\Program Files\PostgreSQL\17\lib\libpq.lib
    set MISSING=1
)

REM Check binary directory
if exist "C:\Program Files\PostgreSQL\17\bin\psql.exe" (
    echo ✓ FOUND: C:\Program Files\PostgreSQL\17\bin\psql.exe
) else (
    echo ❌ MISSING: C:\Program Files\PostgreSQL\17\bin\psql.exe
    set MISSING=1
)

echo.

if defined MISSING (
    echo ====================================================
    echo ❌ SOME FILES ARE MISSING!
    echo ====================================================
    echo.
    echo This might mean:
    echo 1. PostgreSQL 17 is not fully installed
    echo 2. Development components were not selected during installation
    echo 3. PostgreSQL is installed in a different location
    echo.
    echo Solutions:
    echo 1. Reinstall PostgreSQL 17 and select "Development Tools"
    echo 2. Or find your actual PostgreSQL installation path
    echo.
    echo Searching for PostgreSQL in other locations...
    echo.
    
    REM Search for PostgreSQL installations
    for /d %%i in ("C:\Program Files\PostgreSQL\*") do (
        if exist "%%i\include\libpq-fe.h" (
            echo ALTERNATIVE FOUND: %%i\include\libpq-fe.h
        )
    )
    
    for /d %%i in ("C:\Program Files (x86)\PostgreSQL\*") do (
        if exist "%%i\include\libpq-fe.h" (
            echo ALTERNATIVE FOUND: %%i\include\libpq-fe.h
        )
    )
    
) else (
    echo ====================================================
    echo ✓ ALL FILES FOUND! PostgreSQL 17 is properly installed
    echo ====================================================
    echo.
    echo Your CMake should now work with the updated CMakeLists.txt
    echo.
    echo Next steps:
    echo 1. cd build
    echo 2. cmake ..
    echo 3. cmake --build . --target database_test
)

echo.
echo Press any key to exit...
pause >nul