cmake_minimum_required(VERSION 3.10)
project(JsonParser VERSION 1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable testing
enable_testing()
set (PROJECT_FILE "/home/ning/workSpace/Craftrix")
# Find GTest package
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Add include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Add library target
add_library(jsonParser INTERFACE)
target_include_directories(jsonParser INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# Add test executable
add_executable(jsonParserTest jsonParserTest.cpp)
target_link_libraries(jsonParserTest 
    PRIVATE 
    jsonParser
    ${GTEST_BOTH_LIBRARIES}
    pthread
)
target_include_directories(jsonParserTest PUBLIC
    ${PROJECT_FILE}/core
    ${PROJECT_FILE}/tools
)
# Register test
add_test(NAME JsonParserTests COMMAND jsonParserTest)

# Optional: Add a custom target for building and running tests
add_custom_target(check 
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target test
    DEPENDS jsonParserTest
)

# Print status message
message(STATUS "GoogleTest was found and configured")

# Build instructions for the user
message(STATUS "To build and run tests:")
message(STATUS "  mkdir build && cd build")
message(STATUS "  cmake ..")
message(STATUS "  make")
message(STATUS "  ./jsonParserTest")
message(STATUS "  or")
message(STATUS "  make check")