@echo off
echo =====================================================
echo NEXDAY MARKETS - COMPLETE SYSTEM BUILD
echo =====================================================

:: Create build directory
if not exist build mkdir build
cd build

:: Configure with CMake (Debug mode to match your CMakeLists.txt default)
echo Configuring build...
cmake .. -DCMAKE_BUILD_TYPE=Debug -G "Visual Studio 16 2019"

if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed
    pause
    exit /b 1
)

:: Build the project
echo Building project...
cmake --build . --config Debug

if %ERRORLEVEL% neq 0 (
    echo Build failed
    pause
    exit /b 1
)

echo.
echo =====================================================
echo BUILD COMPLETED SUCCESSFULLY
echo =====================================================
echo.
echo Available executables in build\Debug\:
echo   database_test.exe        - Database connectivity test
if exist "Debug\connection_test.exe" echo   connection_test.exe      - IQFeed connection test
if exist "Debug\integrated_test.exe" echo   integrated_test.exe      - Combined system test
if exist "Debug\iqfeed_modular.exe" echo   iqfeed_modular.exe       - Complete IQFeed data collection
if exist "Debug\prediction_manager.exe" echo   prediction_manager.exe   - Epoch Market Advisor predictions
if exist "Debug\prediction_test.exe" echo   prediction_test.exe      - Prediction algorithm testing

echo.
echo Quick test commands:
echo   Database:    cd build\Debug ^&^& database_test.exe
if exist "Debug\prediction_manager.exe" echo   Predictions: cd build\Debug ^&^& prediction_manager.exe

echo.
echo To run specific components, navigate to build\Debug\ directory
echo.
pause