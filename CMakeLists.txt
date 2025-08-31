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
        message(STATUS "✓ Found PostgreSQL 17 manually at: ${PostgreSQL_ROOT}")
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
        message(STATUS "✓ Found PostgreSQL: ${PostgreSQL_VERSION_STRING}")
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
# EXECUTABLE TARGETS
# ==============================================

# IQFeed Connection Test
set(IQFEED_SOURCES
    IQFeedConnection/IQFeedConnection.cpp
)

add_executable(connection_test ${IQFEED_SOURCES})

if(WIN32)
    target_link_libraries(connection_test ${WINDOWS_LIBS})
endif()

# Database Test Executable
set(DATABASE_SOURCES
    Database/database_simple.cpp
    Database/database_test_main.cpp
)

add_executable(database_test ${DATABASE_SOURCES})

# Link PostgreSQL library
target_link_libraries(database_test ${PostgreSQL_LIBRARIES})

if(WIN32)
    target_link_libraries(database_test ${WINDOWS_LIBS})
endif()

# ==============================================
# API TEST EXECUTABLE (simplified version)
# ==============================================

set(API_SOURCES
    IQFeedConnection/IQFeedConnection.cpp
)

add_executable(api_test ${API_SOURCES})

if(WIN32)
    target_link_libraries(api_test ${WINDOWS_LIBS})
endif()

# Set different preprocessor define for api_test
target_compile_definitions(api_test PRIVATE SIMPLE_API_TEST)

# ==============================================
# INTEGRATED TEST (combines IQFeed + Database)
# ==============================================

set(INTEGRATED_SOURCES
    IQFeedConnection/IQFeedConnection.cpp
    Database/database_simple.cpp
)

# Create a simple integrated test main file target
add_executable(integrated_test 
    ${INTEGRATED_SOURCES}
    # This would need an integrated_main.cpp file
)

target_link_libraries(integrated_test ${PostgreSQL_LIBRARIES})

if(WIN32)
    target_link_libraries(integrated_test ${WINDOWS_LIBS})
endif()

# This target is optional - only build if integrated_main.cpp exists
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/integrated_main.cpp")
    set_target_properties(integrated_test PROPERTIES EXCLUDE_FROM_ALL TRUE)
    message(STATUS "Note: integrated_test excluded - create integrated_main.cpp to enable")
endif()

# ==============================================
# BUILD INFORMATION
# ==============================================

message(STATUS "")
message(STATUS "==============================================")
message(STATUS "Nexday Trading System - CMake Configuration")
message(STATUS "==============================================")
message(STATUS "Targets that will be built:")
message(STATUS "  ✓ connection_test    - IQFeed connection testing")
message(STATUS "  ✓ api_test          - Simplified IQFeed API test")
message(STATUS "  ✓ database_test     - PostgreSQL database testing")

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/integrated_main.cpp")
    message(STATUS "  ✓ integrated_test   - Combined IQFeed + Database")
else()
    message(STATUS "  - integrated_test   - (create integrated_main.cpp to enable)")
endif()

message(STATUS "")
message(STATUS "Build commands:")
message(STATUS "  cmake --build . --target connection_test")
message(STATUS "  cmake --build . --target database_test")
message(STATUS "  cmake --build . --target api_test")
message(STATUS "==============================================")
message(STATUS "")