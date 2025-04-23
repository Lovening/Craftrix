CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -pthread
DEBUG_FLAGS = -g -DDEBUG
RELEASE_FLAGS = -O3 -DNDEBUG
GTEST_FLAGS = -lgtest -lgtest_main

# 默认目标
all: test_debug test_release

# 编译调试版本测试
test_debug: memory_pool_test.cpp memory_pool.h
	$(CXX) $(CXXFLAGS) $(DEBUG_FLAGS) memory_pool_test.cpp -o memory_pool_test_debug $(GTEST_FLAGS)

# 编译发布版本测试
test_release: memory_pool_test.cpp memory_pool.h
	$(CXX) $(CXXFLAGS) $(RELEASE_FLAGS) memory_pool_test.cpp -o memory_pool_test_release $(GTEST_FLAGS)

# 运行调试测试
run_debug: test_debug
	./memory_pool_test_debug

# 运行发布测试
run_release: test_release
	./memory_pool_test_release

# 性能测试
benchmark: test_release
	./memory_pool_test_release --gtest_filter=MemoryPoolTest.PerformanceComparison

# 压力测试
stress_test: test_release
	./memory_pool_test_release --gtest_filter=MemoryPoolTest.StressTest

# 运行所有测试
run_all: run_debug run_release

# 清理编译产物
clean:
	rm -f memory_pool_test_debug memory_pool_test_release

.PHONY: all test_debug test_release run_debug run_release benchmark stress_test run_all clean