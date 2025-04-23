cmake_minimum_required(VERSION 3.10)
project(MemoryPool VERSION 1.0 LANGUAGES CXX)

# 设置 C++11 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pthread")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")

# 查找 GTest 包
find_package(GTest REQUIRED)

set (PROJECT_FILE "/home/ning/workSpace/Craftrix")
message(STATUS "CMAKE_SOURCE_DIR = ${CMAKE_SOURCE_DIR}")


# set (CMAKE_INCLUDE_PATH "${PROJECT_FILE}/core/memory")
# 源文件
set(HEADERS
    ${PROJECT_FILE}/core/memory/memoryPool.hpp
)

set(TEST_SOURCES
    ${PROJECT_FILE}/test/memoryPool/memoryPooltest.cpp
)

set(EXAMPLE_SOURCES
    ${PROJECT_FILE}/example/memoryPool/memoryPoolexample.cpp
)



# 添加测试可执行文件 - 调试版本
add_executable(memory_pool_test_debug ${TEST_SOURCES} ${HEADERS})
target_link_libraries(memory_pool_test_debug ${GTEST_LIBRARIES} pthread)
set_target_properties(memory_pool_test_debug PROPERTIES
    COMPILE_FLAGS "${CMAKE_CXX_FLAGS_DEBUG}"
)
target_include_directories(memory_pool_test_debug PUBLIC
    ${PROJECT_FILE}/core/memory
)
# 添加测试可执行文件 - 发布版本
add_executable(memory_pool_test_release ${TEST_SOURCES} ${HEADERS})
target_link_libraries(memory_pool_test_release ${GTEST_LIBRARIES} pthread)
set_target_properties(memory_pool_test_release PROPERTIES
    COMPILE_FLAGS "${CMAKE_CXX_FLAGS_RELEASE}"
)
target_include_directories(memory_pool_test_release PUBLIC
    ${PROJECT_FILE}/core/memory
)
# 添加示例可执行文件
add_executable(memory_pool_example ${EXAMPLE_SOURCES} ${HEADERS})
target_include_directories(memory_pool_example PUBLIC
    ${PROJECT_FILE}/core/memory
)
# 添加测试
enable_testing()

# 添加所有测试
add_test(NAME FullTestDebug COMMAND memory_pool_test_debug)
add_test(NAME FullTestRelease COMMAND memory_pool_test_release)

# 添加性能测试
add_test(
    NAME PerformanceTest 
    COMMAND memory_pool_test_release --gtest_filter=MemoryPoolTest.PerformanceComparison
)

# 添加压力测试
add_test(
    NAME StressTest 
    COMMAND memory_pool_test_release --gtest_filter=MemoryPoolTest.StressTest
)

# 自定义目标
add_custom_target(run_debug
    COMMAND memory_pool_test_debug
    DEPENDS memory_pool_test_debug
    COMMENT "运行调试版本测试"
)

add_custom_target(run_release
    COMMAND memory_pool_test_release
    DEPENDS memory_pool_test_release
    COMMENT "运行发布版本测试"
)

add_custom_target(run_all
    COMMAND make run_debug
    COMMAND make run_release
    COMMENT "运行所有测试"
)

add_custom_target(benchmark
    COMMAND memory_pool_test_release --gtest_filter=MemoryPoolTest.PerformanceComparison
    DEPENDS memory_pool_test_release
    COMMENT "运行性能测试"
)

add_custom_target(stress_test
    COMMAND memory_pool_test_release --gtest_filter=MemoryPoolTest.StressTest
    DEPENDS memory_pool_test_release
    COMMENT "运行压力测试"
)

# 输出配置信息
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ Flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "Debug Flags: ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "Release Flags: ${CMAKE_CXX_FLAGS_RELEASE}")