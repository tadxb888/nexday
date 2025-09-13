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

# ==============================================
# BUILD INFORMATION
# ==============================================

message(STATUS "")
message(STATUS "==============================================")
message(STATUS "Nexday Trading System - Minimal Build Configuration")
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

message(STATUS "")
message(STATUS "Quick Start Commands:")
message(STATUS "  1. Build:     cmake --build .")
message(STATUS "  2. Test DB:   cmake --build . --target test_database")
message(STATUS "  3. Init DB:   cmake --build . --target init_database")
message(STATUS "")
message(STATUS "Manual Test Commands:")
message(STATUS "  Database:     .\\Debug\\database_test.exe")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/IQFeedConnection/IQFeedConnection.cpp")
    message(STATUS "  IQFeed:       .\\Debug\\connection_test.exe")
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/integrated_test_main.cpp")
    message(STATUS "  Integration:  .\\Debug\\integrated_test.exe")
endif()
message(STATUS "==============================================")
message(STATUS "")
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
    )
    
    if(WIN32)
        target_link_libraries(iqfeed_modular ${WINDOWS_LIBS})
    endif()
    
    target_compile_definitions(iqfeed_modular PRIVATE MODULAR_IQFEED_SYSTEM)
    message(STATUS "✅ iqfeed_modular - will be built with complete modular architecture")
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
endif()