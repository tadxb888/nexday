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
# DEFINE SOURCE FILE GROUPS
# ==============================================

# Database sources
set(DATABASE_SOURCES
    Database/database_simple.cpp
)

# IQFeed Connection sources (COMPLETE SET)
set(IQFEED_SOURCES
    IQFeedConnection/IQFeedConnectionManager.cpp
    IQFeedConnection/Logger.cpp
    IQFeedConnection/HistoricalDataFetcher.cpp
    IQFeedConnection/DailyDataFetcher.cpp
)

# Check for additional IQFeed fetchers and add if they exist
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/IQFeedConnection/FifteenMinDataFetcher.cpp")
    list(APPEND IQFEED_SOURCES IQFeedConnection/FifteenMinDataFetcher.cpp)
    message(STATUS "  ✅ Including FifteenMinDataFetcher.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/IQFeedConnection/ThirtyMinDataFetcher.cpp")
    list(APPEND IQFEED_SOURCES IQFeedConnection/ThirtyMinDataFetcher.cpp)
    message(STATUS "  ✅ Including ThirtyMinDataFetcher.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/IQFeedConnection/OneHourDataFetcher.cpp")
    list(APPEND IQFEED_SOURCES IQFeedConnection/OneHourDataFetcher.cpp)
    message(STATUS "  ✅ Including OneHourDataFetcher.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/IQFeedConnection/TwoHourDataFetcher.cpp")
    list(APPEND IQFEED_SOURCES IQFeedConnection/TwoHourDataFetcher.cpp)
    message(STATUS "  ✅ Including TwoHourDataFetcher.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/IQFeedConnection/FetchScheduler.cpp")
    list(APPEND IQFEED_SOURCES IQFeedConnection/FetchScheduler.cpp)
    message(STATUS "  ✅ Including FetchScheduler.cpp")
endif()

# Combined core system sources
set(CORE_SYSTEM_SOURCES
    ${DATABASE_SOURCES}
    ${IQFEED_SOURCES}
)

# ==============================================
# 🎯 PRIMARY TARGET: COMPLETE END-TO-END PIPELINE
# ==============================================

add_executable(complete_pipeline 
    Predictions/CompletePipeline.cpp
    ${CORE_SYSTEM_SOURCES}
)

target_link_libraries(complete_pipeline ${PostgreSQL_LIBRARIES})

if(WIN32)
    target_link_libraries(complete_pipeline ${WINDOWS_LIBS})
endif()

# Set compiler flags for complete pipeline
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(complete_pipeline PRIVATE /W4)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(complete_pipeline PRIVATE -Wall -Wextra -Wpedantic)
endif()

message(STATUS "🚀 complete_pipeline - COMPLETE END-TO-END PIPELINE configured")

# ==============================================
# YOUR EXISTING WORKING TESTS (UNCHANGED)
# ==============================================

# Database Test Executable
add_executable(database_test 
    Database/database_simple.cpp
    Database/database_test_main.cpp
)
target_link_libraries(database_test ${PostgreSQL_LIBRARIES})
if(WIN32)
    target_link_libraries(database_test ${WINDOWS_LIBS})
endif()

# Minimal prediction test (your working test)
add_executable(minimal_test 
    Database/database_simple.cpp
    minimal_prediction_test.cpp
)
target_link_libraries(minimal_test ${PostgreSQL_LIBRARIES})
if(WIN32)
    target_link_libraries(minimal_test ${WINDOWS_LIBS})
endif()

# EMA calculation test (your working test)
add_executable(ema_test 
    ema_test.cpp
)

# Historical EMA test with real data (your working test)
add_executable(historical_ema_test 
    Database/database_simple.cpp
    historical_ema_test.cpp
)
target_link_libraries(historical_ema_test ${PostgreSQL_LIBRARIES})
if(WIN32)
    target_link_libraries(historical_ema_test ${WINDOWS_LIBS})
endif()

# ==============================================
# CONDITIONAL TARGETS (IF FILES EXIST)
# ==============================================

# Original main application (if exists)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/IQFeedConnection/main.cpp")
    add_executable(nexday_main 
        IQFeedConnection/main.cpp
        ${CORE_SYSTEM_SOURCES}
    )
    
    target_link_libraries(nexday_main ${PostgreSQL_LIBRARIES})
    
    if(WIN32)
        target_link_libraries(nexday_main ${WINDOWS_LIBS})
    endif()
    
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(nexday_main PRIVATE /W4)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(nexday_main PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    
    message(STATUS "✅ nexday_main - Original main application configured")
else()
    message(STATUS "➖ nexday_main - IQFeedConnection/main.cpp not found")
endif()

# ==============================================
# CUSTOM TARGETS FOR EASY EXECUTION
# ==============================================

# 🎯 Primary target: Run the COMPLETE PIPELINE
add_custom_target(run_complete_pipeline
    COMMAND $<TARGET_FILE:complete_pipeline>
    DEPENDS complete_pipeline
    COMMENT "🚀 Running Complete End-to-End Pipeline: IQFeed → Data → Predictions → Errors"
)

# Test your existing working components
add_custom_target(test_database
    COMMAND $<TARGET_FILE:database_test>
    DEPENDS database_test
    COMMENT "Testing database connectivity"
)

add_custom_target(test_ema
    COMMAND $<TARGET_FILE:ema_test>
    DEPENDS ema_test
    COMMENT "Testing EMA calculation"
)

add_custom_target(test_minimal
    COMMAND $<TARGET_FILE:minimal_test>
    DEPENDS minimal_test
    COMMENT "Running minimal prediction test"
)

add_custom_target(test_historical_ema
    COMMAND $<TARGET_FILE:historical_ema_test>
    DEPENDS historical_ema_test
    COMMENT "Testing EMA with real historical data"
)

# Custom target to verify all your working components first
add_custom_target(verify_components
    COMMAND echo "🔍 Verifying all working components..."
    COMMAND $<TARGET_FILE:database_test>
    COMMAND $<TARGET_FILE:ema_test>
    COMMAND $<TARGET_FILE:minimal_test>
    COMMAND $<TARGET_FILE:historical_ema_test>
    DEPENDS database_test ema_test minimal_test historical_ema_test
    COMMENT "Verifying all working components before running complete pipeline"
)

# ==============================================
# BUILD INFORMATION & INSTRUCTIONS
# ==============================================

message(STATUS "")
message(STATUS "==============================================")
message(STATUS "NEXDAY MARKETS - COMPLETE PIPELINE BUILD")
message(STATUS "==============================================")
message(STATUS "")
message(STATUS "🎯 PRIMARY TARGET:")
message(STATUS "  complete_pipeline     - COMPLETE END-TO-END SYSTEM")
message(STATUS "                          Connects → Fetches → Stores → Predicts → Validates")
message(STATUS "")
message(STATUS "✅ YOUR WORKING TESTS (unchanged):")
message(STATUS "  database_test         - PostgreSQL connectivity")
message(STATUS "  minimal_test          - Basic prediction test") 
message(STATUS "  ema_test              - EMA calculation verification")
message(STATUS "  historical_ema_test   - EMA + database integration")

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/IQFeedConnection/main.cpp")
    message(STATUS "  nexday_main           - Your original main application")
endif()

message(STATUS "")
message(STATUS "==============================================")
message(STATUS "📋 BUILD & RUN INSTRUCTIONS:")
message(STATUS "")
message(STATUS "1. BUILD EVERYTHING:")
message(STATUS "   cmake --build .")
message(STATUS "")
message(STATUS "2. VERIFY YOUR WORKING COMPONENTS FIRST:")
message(STATUS "   cmake --build . --target verify_components")
message(STATUS "")
message(STATUS "3. RUN THE COMPLETE PIPELINE:")
message(STATUS "   cmake --build . --target run_complete_pipeline")
message(STATUS "   OR directly:")
message(STATUS "   .\\Debug\\complete_pipeline.exe")
message(STATUS "")
message(STATUS "==============================================")
message(STATUS "🎯 MAIN EXECUTABLE: complete_pipeline.exe")
message(STATUS "")
message(STATUS "ONE EXECUTION RUNS THE COMPLETE SYSTEM!")
message(STATUS "==============================================")
message(STATUS "")