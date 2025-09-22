cmake_minimum_required(VERSION 3.16)
project(nexday)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Print build information
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER}")

# ==============================================
# FIND POSTGRESQL (libpq) - Windows PostgreSQL 17 Fix
# ==============================================

# Windows: Explicitly set PostgreSQL paths for version 17
if(WIN32)
    message(STATUS "Configuring for Windows with PostgreSQL 17...")
    
    # Set explicit paths for PostgreSQL 17
    set(PostgreSQL_ROOT "C:/Program Files/PostgreSQL/17")
    set(PostgreSQL_INCLUDE_DIRS "${PostgreSQL_ROOT}/include")
    set(PostgreSQL_LIBRARIES "${PostgreSQL_ROOT}/lib/libpq.lib")
    
    # Check if files exist
    if(EXISTS "${PostgreSQL_INCLUDE_DIRS}/libpq-fe.h" AND EXISTS "${PostgreSQL_LIBRARIES}")
        set(PostgreSQL_FOUND TRUE)
        set(PostgreSQL_VERSION_STRING "17.x")
        message(STATUS "✅ Found PostgreSQL 17 manually at: ${PostgreSQL_ROOT}")
        message(STATUS "  Include dir: ${PostgreSQL_INCLUDE_DIRS}")
        message(STATUS "  Libraries: ${PostgreSQL_LIBRARIES}")
    else()
        message(STATUS "❌ PostgreSQL 17 files not found at expected location")
        message(STATUS "  Checked include: ${PostgreSQL_INCLUDE_DIRS}/libpq-fe.h")
        message(STATUS "  Checked library: ${PostgreSQL_LIBRARIES}")
        message(FATAL_ERROR "Please verify PostgreSQL 17 installation")
    endif()
    
    # Add Windows socket libraries
    set(WINDOWS_LIBS ws2_32 wsock32)
    
    # Add compile definitions for Windows
    add_definitions(-DWIN32_LEAN_AND_MEAN -DNOMINMAX)
    
else()
    # Linux/Mac: Use standard find_package
    find_package(PostgreSQL REQUIRED)
    if(PostgreSQL_FOUND)
        message(STATUS "✅ Found PostgreSQL: ${PostgreSQL_VERSION_STRING}")
        message(STATUS "  Include dir: ${PostgreSQL_INCLUDE_DIRS}")
        message(STATUS "  Libraries: ${PostgreSQL_LIBRARIES}")
    endif()
endif()

# ==============================================
# INCLUDE DIRECTORIES
# ==============================================

# Add PostgreSQL include directory
include_directories(${PostgreSQL_INCLUDE_DIRS})

# Add project include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/IQFeedConnection
    ${CMAKE_CURRENT_SOURCE_DIR}/Database
    ${CMAKE_CURRENT_SOURCE_DIR}/Predictions
)

# ==============================================
# EXECUTABLE TARGETS - ONLY EXISTING FILES
# ==============================================

# Database Test Executable (Your existing file)
add_executable(database_test 
    Database/database_simple.cpp
    Database/database_test_main.cpp
)

# Link PostgreSQL library
target_link_libraries(database_test ${PostgreSQL_LIBRARIES})

if(WIN32)
    target_link_libraries(database_test ${WINDOWS_LIBS})
endif()

# Set compiler flags
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(database_test PRIVATE /W4)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(database_test PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ==============================================
# CONDITIONAL TARGETS - BUILD ONLY IF FILES EXIST
# ==============================================

# IQFeed Connection Test (if IQFeedConnection.cpp exists)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/IQFeedConnection/IQFeedConnection.cpp")
    add_executable(connection_test 
        IQFeedConnection/IQFeedConnection.cpp
    )
    
    if(WIN32)
        target_link_libraries(connection_test ${WINDOWS_LIBS})
    endif()
    
    target_compile_definitions(connection_test PRIVATE CONNECTION_TEST_MODE)
    message(STATUS "✅ connection_test - will be built")
else()
    message(STATUS "➖ connection_test - IQFeedConnection.cpp not found")
endif()

# Integrated Test (if integrated_test_main.cpp exists)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/integrated_test_main.cpp")
    add_executable(integrated_test 
        Database/database_simple.cpp
        integrated_test_main.cpp
    )
    
    # DO NOT add IQFeedConnection.cpp as it has its own main() function
    # The integrated_test should use integrated_test_main.cpp's main() only
    
    target_link_libraries(integrated_test ${PostgreSQL_LIBRARIES})
    
    if(WIN32)
        target_link_libraries(integrated_test ${WINDOWS_LIBS})
    endif()
    
    target_compile_definitions(integrated_test PRIVATE INTEGRATED_TEST_MODE)
    message(STATUS "✅ integrated_test - will be built (without IQFeedConnection.cpp to avoid main() conflict)")
else()
    message(STATUS "➖ integrated_test - integrated_test_main.cpp not found")
endif()

# ==============================================
# MODULAR IQFEED HISTORICAL DATA SYSTEM  
# ==============================================

# IQFeed Modular System (if all source files exist)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/IQFeedConnection/IQFeedConnectionManager.cpp" AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/IQFeedConnection/Logger.cpp" AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/IQFeedConnection/HistoricalDataFetcher.cpp" AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/IQFeedConnection/DailyDataFetcher.cpp")
   
    add_executable(iqfeed_modular
        IQFeedConnection/main.cpp
        IQFeedConnection/IQFeedConnectionManager.cpp
        IQFeedConnection/Logger.cpp
        IQFeedConnection/HistoricalDataFetcher.cpp
        IQFeedConnection/DailyDataFetcher.cpp
        IQFeedConnection/FifteenMinDataFetcher.cpp
        IQFeedConnection/ThirtyMinDataFetcher.cpp
        IQFeedConnection/OneHourDataFetcher.cpp
        IQFeedConnection/TwoHourDataFetcher.cpp
        IQFeedConnection/FetchScheduler.cpp
        Database/database_simple.cpp
    )
    
    # Link PostgreSQL library for database access
    target_link_libraries(iqfeed_modular ${PostgreSQL_LIBRARIES})
    
    if(WIN32)
        target_link_libraries(iqfeed_modular ${WINDOWS_LIBS})
    endif()
    
    target_compile_definitions(iqfeed_modular PRIVATE MODULAR_IQFEED_SYSTEM)
    message(STATUS "✅ iqfeed_modular - will be built with complete modular architecture + scheduler")
    message(STATUS "    Note: Now using 2-hour intervals instead of 4-hour (IQFeed officially supported)")
else()
    message(STATUS "➖ iqfeed_modular - missing source files for modular system")
    message(STATUS "    Required files:")
    message(STATUS "      - IQFeedConnection/IQFeedConnectionManager.cpp")
    message(STATUS "      - IQFeedConnection/Logger.cpp") 
    message(STATUS "      - IQFeedConnection/HistoricalDataFetcher.cpp")
    message(STATUS "      - IQFeedConnection/DailyDataFetcher.cpp")
    message(STATUS "      - IQFeedConnection/FifteenMinDataFetcher.cpp")
    message(STATUS "      - IQFeedConnection/ThirtyMinDataFetcher.cpp")
    message(STATUS "      - IQFeedConnection/OneHourDataFetcher.cpp")
    message(STATUS "      - IQFeedConnection/TwoHourDataFetcher.cpp")
    message(STATUS "      - IQFeedConnection/FetchScheduler.cpp")
    message(STATUS "      - Database/database_simple.cpp")
endif()

# ==============================================
# MARKET PREDICTIONS MODULE - EPOCH MARKET ADVISOR
# ==============================================

# Market Predictions System (if all source files exist)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Predictions/MarketPredictionEngine.cpp" AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Predictions/PredictionTypes.h" AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Predictions/BusinessDayCalculator.h" AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Predictions/MarketPredictionEngine.h")
   
    # Create prediction engine library first
    add_library(NexdayPredictionEngine STATIC
        Predictions/MarketPredictionEngine.cpp
    )
    
    # Link PostgreSQL library to prediction engine
    target_link_libraries(NexdayPredictionEngine ${PostgreSQL_LIBRARIES})
    
    if(WIN32)
        target_link_libraries(NexdayPredictionEngine ${WINDOWS_LIBS})
    endif()
    
    # Create main prediction manager executable
    add_executable(prediction_manager
        Predictions/MarketPredictionManager.cpp
        Database/database_simple.cpp
    )
    
    # Link prediction engine and PostgreSQL libraries
    target_link_libraries(prediction_manager 
        NexdayPredictionEngine
        ${PostgreSQL_LIBRARIES}
    )
    
    if(WIN32)
        target_link_libraries(prediction_manager ${WINDOWS_LIBS})
    endif()
    
    target_compile_definitions(prediction_manager PRIVATE NEXDAY_PREDICTIONS_MODULE)
    
    message(STATUS "✅ prediction_manager - Epoch Market Advisor engine will be built")
    message(STATUS "    Features: Daily OHLC + Intraday High/Low predictions")
    message(STATUS "    Algorithm: Model 1 Standard EMA with 15-bar minimum")
    message(STATUS "    Business Logic: Weekend skipping, confidence scoring")
else()
    message(STATUS "➖ prediction_manager - missing source files for Market Predictions")
    message(STATUS "    Required files:")
    message(STATUS "      - Predictions/PredictionTypes.h")
    message(STATUS "      - Predictions/BusinessDayCalculator.h") 
    message(STATUS "      - Predictions/MarketPredictionEngine.h")
    message(STATUS "      - Predictions/MarketPredictionEngine.cpp")
    message(STATUS "      - Predictions/MarketPredictionManager.cpp")
    message(STATUS "      - Predictions/NexdayMarketPredictions.h")
endif()

# Create standalone prediction test (if test file exists)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Predictions/PredictionTest.cpp" AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Predictions/MarketPredictionEngine.cpp")
   
    add_executable(prediction_test
        Predictions/PredictionTest.cpp
        Predictions/MarketPredictionEngine.cpp
        Database/database_simple.cpp
    )
    
    target_link_libraries(prediction_test ${PostgreSQL_LIBRARIES})
    
    if(WIN32)
        target_link_libraries(prediction_test ${WINDOWS_LIBS})
    endif()
    
    target_compile_definitions(prediction_test PRIVATE PREDICTION_TEST_MODE)
    message(STATUS "✅ prediction_test - EMA calculation testing will be built")
else()
    message(STATUS "➖ prediction_test - (optional) create Predictions/PredictionTest.cpp for standalone testing")
endif()

# ==============================================
# INTEGRATED PREDICTION ENGINE - NEW MODULE
# ==============================================

# Integrated Prediction Engine (if source files exist or will be created)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/integrated_prediction_main.cpp" OR
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/IntegratedMarketPredictionEngine.cpp")
   
    add_executable(integrated_prediction_engine
        integrated_prediction_main.cpp
        IntegratedMarketPredictionEngine.cpp
        Database/database_simple.cpp
        IQFeedConnection/IQFeedConnectionManager.cpp
        IQFeedConnection/Logger.cpp
    )
    
    # Link PostgreSQL library
    target_link_libraries(integrated_prediction_engine ${PostgreSQL_LIBRARIES})
    
    if(WIN32)
        target_link_libraries(integrated_prediction_engine ${WINDOWS_LIBS})
    endif()
    
    target_compile_definitions(integrated_prediction_engine PRIVATE INTEGRATED_PREDICTIONS_MODULE)
    
    message(STATUS "✅ integrated_prediction_engine - NEW! Fully integrated prediction system")
    message(STATUS "    Features: Uses REAL database data + IQFeed integration")
    message(STATUS "    Algorithm: Model 1 Standard EMA with proper historical data")
    message(STATUS "    Integration: Database historical_fetch_* tables + predictions_all_symbols")
    message(STATUS "    Replaces: Isolated prediction_manager with full data pipeline integration")
    
else()
    message(STATUS "➖ integrated_prediction_engine - create integrated_prediction_main.cpp and IntegratedMarketPredictionEngine.cpp")
    message(STATUS "    This will replace your isolated prediction_manager with full database/IQFeed integration")
endif()

# ==============================================
# CUSTOM TARGETS FOR DATABASE MANAGEMENT
# ==============================================

# Custom target to initialize database
add_custom_target(init_database
    COMMAND echo "Initializing Nexday Trading Database..."
    COMMAND psql -h localhost -p 5432 -U nexday_user -d nexday_trading -f "${CMAKE_CURRENT_SOURCE_DIR}/nexday_schema.sql"
    COMMENT "Initializing database with complete schema"
)

# Custom target to run database test
add_custom_target(test_database
    COMMAND $<TARGET_FILE:database_test>
    DEPENDS database_test
    COMMENT "Running database connectivity test"
)

# Custom target to run integrated predictions (if available)
if(TARGET integrated_prediction_engine)
    add_custom_target(run_integrated_predictions
        COMMAND $<TARGET_FILE:integrated_prediction_engine>
        DEPENDS integrated_prediction_engine
        COMMENT "Running integrated prediction engine with real data pipeline"
    )
endif()

# ==============================================
# BUILD INFORMATION
# ==============================================

message(STATUS "")
message(STATUS "==============================================")
message(STATUS "Nexday Trading System - Complete Build Configuration")
message(STATUS "==============================================")
message(STATUS "Targets that will be built:")
message(STATUS "  ✅ database_test     - PostgreSQL database testing")

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/IQFeedConnection/IQFeedConnection.cpp")
    message(STATUS "  ✅ connection_test   - IQFeed connection testing")
else()
    message(STATUS "  ➖ connection_test   - (create IQFeedConnection/IQFeedConnection.cpp)")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/integrated_test_main.cpp")
    message(STATUS "  ✅ integrated_test   - Combined system test")
else()
    message(STATUS "  ➖ integrated_test   - (create integrated_test_main.cpp)")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/IQFeedConnection/IQFeedConnectionManager.cpp")
    message(STATUS "  ✅ iqfeed_modular    - Complete IQFeed data collection system")
else()
    message(STATUS "  ➖ iqfeed_modular    - (missing modular IQFeed files)")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Predictions/MarketPredictionEngine.cpp")
    message(STATUS "  ✅ prediction_manager - Epoch Market Advisor prediction engine")
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Predictions/PredictionTest.cpp")
        message(STATUS "  ✅ prediction_test   - Prediction algorithm testing")
    else()
        message(STATUS "  ➖ prediction_test   - (create Predictions/PredictionTest.cpp)")
    endif()
else()
    message(STATUS "  ➖ prediction_manager - (create Predictions/ directory with source files)")
endif()

if(TARGET integrated_prediction_engine)
    message(STATUS "  🔥 integrated_prediction_engine - RECOMMENDED! Fully integrated system")
else()
    message(STATUS "  ➖ integrated_prediction_engine - (create integrated prediction files)")
endif()

message(STATUS "")
message(STATUS "Quick Start Commands:")
message(STATUS "  1. Build:     cmake --build .")
message(STATUS "  2. Test DB:   cmake --build . --target test_database")
message(STATUS "  3. Init DB:   cmake --build . --target init_database")
if(TARGET integrated_prediction_engine)
    message(STATUS "  4. Predict:   cmake --build . --target run_integrated_predictions")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Predictions/MarketPredictionEngine.cpp")
    message(STATUS "  4. Predict:   cmake --build . --target run_predictions")
endif()
message(STATUS "")
message(STATUS "Manual Test Commands:")
message(STATUS "  Database:     .\\Debug\\database_test.exe")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/IQFeedConnection/IQFeedConnection.cpp")
    message(STATUS "  IQFeed:       .\\Debug\\connection_test.exe")
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/integrated_test_main.cpp")
    message(STATUS "  Integration:  .\\Debug\\integrated_test.exe")
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/IQFeedConnection/IQFeedConnectionManager.cpp")
    message(STATUS "  IQFeed Mod:   .\\Debug\\iqfeed_modular.exe")
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Predictions/MarketPredictionEngine.cpp")
    message(STATUS "  Predictions:  .\\Debug\\prediction_manager.exe (ISOLATED)")
endif()
if(TARGET integrated_prediction_engine)
    message(STATUS "  NEW Predict:  .\\Debug\\integrated_prediction_engine.exe (INTEGRATED)")
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Predictions/PredictionTest.cpp")
    message(STATUS "  Pred Test:    .\\Debug\\prediction_test.exe")
endif()
message(STATUS "==============================================")
message(STATUS "")