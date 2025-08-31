cmake_minimum_required(VERSION 3.16)

# Project configuration
project(Nexday_IQFeed_Trading_System 
    VERSION 1.0.0 
    DESCRIPTION "IQFeed Trading System with Market Predictions"
    LANGUAGES CXX
)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build configuration
set(CMAKE_BUILD_TYPE Debug)

# Windows-specific settings
if(WIN32)
    # Link Windows socket library
    set(PLATFORM_LIBS ws2_32)
    
    # Windows-specific definitions
    add_definitions(-D_WIN32_WINNT=0x0601)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/IQFeedConnection)

# IQFeed Connection Test executable
add_executable(connection_test 
    IQFeedConnection/IQFeedConnection.cpp
)
target_link_libraries(connection_test ${PLATFORM_LIBS})

# Future executables (for when you expand the project)
# add_executable(historical_test
#     IQFeedConnection/HistoricalDataTest.cpp
#     IQFeedConnection/IQFeedConnection.cpp
# )
# target_link_libraries(historical_test ${PLATFORM_LIBS})

# Main trading system (for future expansion)
# add_executable(trading_system
#     IQFeedConnection/main.cpp
#     IQFeedConnection/IQFeedConnection.cpp
# )
# target_link_libraries(trading_system ${PLATFORM_LIBS})

# Add this executable to your existing CMakeLists.txt
add_executable(api_test
    tests/api_test.cpp
)
target_link_libraries(api_test ${PLATFORM_LIBS})

# Compiler-specific options
if(MSVC)
    target_compile_options(connection_test PRIVATE /W4)
else()
    target_compile_options(connection_test PRIVATE -Wall -Wextra -pedantic)
endif()

# Print build information
message(STATUS "")
message(STATUS "=== Nexday IQFeed Trading System ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Source file: IQFeedConnection/IQFeedConnection.cpp")
message(STATUS "====================================")
message(STATUS "")