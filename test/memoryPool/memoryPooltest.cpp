#include <gtest/gtest.h>
#include "memoryPool.hpp" // 包含优化后的内存池头文件
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <algorithm>
#include <random>

// 用于测试的简单类
class TestItem {
public:
    TestItem() : value(0), str("default") {}
    TestItem(int v, std::string s) : value(v), str(std::move(s)) {}
    
    bool operator==(const TestItem& other) const {
        return value == other.value && str == other.str;
    }
    
    int getValue() const { return value; }
    const std::string& getString() const { return str; }
    
private:
    int value;
    std::string str;
};

// 带析构计数器的类
class CountedItem {
public:
    CountedItem() { ++constructCount; }
    ~CountedItem() { ++destructCount; }
    
    static void resetCounters() {
        constructCount = 0;
        destructCount = 0;
    }
    
    static std::atomic<int> constructCount;
    static std::atomic<int> destructCount;
};

std::atomic<int> CountedItem::constructCount(0);
std::atomic<int> CountedItem::destructCount(0);

// 基本功能测试
TEST(MemoryPoolTest, BasicFunctionality) {
    MemoryPool<TestItem> pool(10);
    
    // 测试基本分配和构造
    TestItem* item1 = pool.allocate();
    new (item1) TestItem(42, "test1");
    
    TestItem* item2 = pool.construct(84, "test2");
    
    // 验证对象值
    EXPECT_EQ(item1->getValue(), 42);
    EXPECT_EQ(item1->getString(), "test1");
    EXPECT_EQ(item2->getValue(), 84);
    EXPECT_EQ(item2->getString(), "test2");
    
    // 测试销毁和释放
    item1->~TestItem();
    pool.deallocate(item1);
    pool.destroy(item2);
    
    // 检查内存计数
    EXPECT_EQ(pool.free_count(), 10);
    EXPECT_EQ(pool.total_count(), 10);
    EXPECT_EQ(pool.allocated_count(), 0);
}

// 测试智能指针接口
TEST(MemoryPoolTest, SmartPointer) {
    MemoryPool<TestItem> pool(10);
    
    {
        auto item = pool.make_shared(42, "smart");
        EXPECT_EQ(item->getValue(), 42);
        EXPECT_EQ(item->getString(), "smart");
        
        EXPECT_EQ(pool.allocated_count(), 1);
    }
    
    // 智能指针超出作用域后，对象应该被销毁
    EXPECT_EQ(pool.allocated_count(), 0);
    EXPECT_EQ(pool.free_count(), 10);
}

// 测试构造/析构正确性
TEST(MemoryPoolTest, ConstructionDestruction) {
    CountedItem::resetCounters();
    
    {
        MemoryPool<CountedItem> pool(5);
        // 分配并构造多个对象
        std::vector<CountedItem*> items;
        for (int i = 0; i < 5; ++i) {
            items.push_back(pool.construct());
        }
        
        EXPECT_EQ(CountedItem::constructCount, 5);
        EXPECT_EQ(CountedItem::destructCount, 0);

        // 销毁一半对象
        for (size_t i = 0; i < items.size() / 2; ++i) {
            pool.destroy(items[i]);
            items[i] = nullptr;
        }

        EXPECT_EQ(CountedItem::destructCount, 2);
        // 在 Debug 模式下，剩余对象会在 pool 析构时自动销毁
        // 在 Release 模式下，我们需要手动清理剩余对象
#ifdef NDEBUG
        for (auto item : items) {
            if (item) pool.destroy(item);
        }
#endif
    }
    
    // pool析构后，检查是否所有对象都被销毁
    EXPECT_EQ(CountedItem::constructCount, 5);
    EXPECT_EQ(CountedItem::destructCount, 5);
}

// 测试内存分配溢出
TEST(MemoryPoolTest, Overflow) {
    MemoryPool<TestItem, false> pool(5, 1); // 最多只能有5个对象，只允许1个内存块
    
    std::vector<TestItem*> items;
    
    // 应该能成功分配5个对象
    for (int i = 0; i < 5; ++i) {
        items.push_back(pool.construct(i, "test"));
    }
    
    // 第6个对象应该会抛出异常
    EXPECT_THROW(pool.allocate(), std::bad_alloc);
    
    // 释放一个对象后，应该可以再次分配
    pool.destroy(items.back());
    items.pop_back();
    
    TestItem* newItem = pool.allocate();
    EXPECT_NE(newItem, nullptr);
    new (newItem) TestItem(99, "new");
    items.push_back(newItem);
    
    // 清理
    for (auto item : items) {
        pool.destroy(item);
    }
}

// 测试预分配功能
TEST(MemoryPoolTest, Reserve) {
    MemoryPool<TestItem> pool(10);
    
    // 初始状态
    EXPECT_EQ(pool.total_count(), 10);
    
    // 预分配2个块
    pool.reserve(3);
    EXPECT_EQ(pool.total_count(), 30);
    EXPECT_EQ(pool.free_count(), 30);
    
    // 分配一些对象
    std::vector<TestItem*> items;
    for (int i = 0; i < 15; ++i) {
        items.push_back(pool.construct(i, "reserved"));
    }
    
    EXPECT_EQ(pool.allocated_count(), 15);
    EXPECT_EQ(pool.free_count(), 15);
    
    // 清理
    for (auto item : items) {
        pool.destroy(item);
    }
}

// 多线程测试 - 启用线程本地存储
TEST(MemoryPoolTest, MultithreadedWithTLS) {
    const int threadCount = 4;
    const int itemsPerThread = 1000;
    
    MemoryPool<TestItem, true> pool(itemsPerThread);
    pool.reserve(threadCount + 1); // 预分配足够的内存
    
    std::atomic<int> readyCount(0);
    std::vector<std::thread> threads;
    
    auto threadFunc = [&](int threadId) {
        std::vector<TestItem*> items;
        items.reserve(itemsPerThread);
        
        // 等待所有线程准备就绪
        readyCount.fetch_add(1);
        while (readyCount.load() < threadCount) {
            std::this_thread::yield();
        }
        
        // 分配和构造对象
        for (int i = 0; i < itemsPerThread; ++i) {
            items.push_back(pool.construct(threadId * 10000 + i, "thread"));
        }
        
        // 随机顺序释放一半对象
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(items.begin(), items.end(), g);
        
        for (int i = 0; i < itemsPerThread / 2; ++i) {
            pool.destroy(items[i]);
            items[i] = nullptr;
        }
        
        // 重新分配
        for (int i = 0; i < itemsPerThread / 2; ++i) {
            if (items[i] == nullptr) {
                items[i] = pool.construct(threadId * 20000 + i, "realloc");
            }
        }
        
        // 清理所有对象
        for (auto item : items) {
            if (item) pool.destroy(item);
        }
    };
    
    // 创建并启动所有线程
    for (int i = 0; i < threadCount; ++i) {
        threads.emplace_back(threadFunc, i);
    }
    
    // 等待所有线程完成
    for (auto& t : threads) {
        t.join();
    }
    
    // 所有对象应该已释放
    EXPECT_EQ(pool.allocated_count(), 0);
    EXPECT_GE(pool.total_count(), threadCount * itemsPerThread);
}

// 性能比较测试：标准分配器 vs 内存池
TEST(MemoryPoolTest, PerformanceComparison) {
    const int iterations = 10000000;
    std::vector<TestItem*> stdItems;
    std::vector<TestItem*> poolItems;
    
    MemoryPool<TestItem> pool(iterations / 10);
    pool.reserve(20); // 预分配足够的内存
    
    // 测试标准分配器性能
    auto stdStart = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        TestItem* item = new TestItem(i, "std");
        stdItems.push_back(item);
    }
    
    for (int i = 0; i < iterations; ++i) {
        delete stdItems[i];
    }
    stdItems.clear();
    
    auto stdEnd = std::chrono::high_resolution_clock::now();
    auto stdDuration = std::chrono::duration_cast<std::chrono::milliseconds>(stdEnd - stdStart).count();
    
    // 测试内存池性能
    auto poolStart = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        TestItem* item = pool.construct(i, "pool");
        poolItems.push_back(item);
    }
    
    for (int i = 0; i < iterations; ++i) {
        pool.destroy(poolItems[i]);
    }
    poolItems.clear();
    
    auto poolEnd = std::chrono::high_resolution_clock::now();
    auto poolDuration = std::chrono::duration_cast<std::chrono::milliseconds>(poolEnd - poolStart).count();
    
    std::cout << "Performance comparison for " << iterations << " allocations and deallocations:" << std::endl;
    std::cout << "  Standard allocator: " << stdDuration << "ms" << std::endl;
    std::cout << "  Memory pool:        " << poolDuration << "ms" << std::endl;
    std::cout << "  Speedup:            " << (float)stdDuration / poolDuration << "x" << std::endl;
    
    // 内存池应该比标准分配器快
    // EXPECT_LT(poolDuration, stdDuration);
    EXPECT_LE(poolDuration, stdDuration);
}

// 内存泄漏测试
#ifdef NDEBUG
TEST(MemoryPoolTest, MemoryLeakDetection) {
    // 仅在Debug模式下测试内存泄漏检测
    GTEST_SKIP() << "Memory leak detection test skipped in Release mode";
}
#else
TEST(MemoryPoolTest, MemoryLeakDetection) {
    MemoryPool<TestItem>* pool = new MemoryPool<TestItem>(5);
    
    // 分配但不释放
    TestItem* item = pool->construct(42, "leak");
    
    // 如果内存池析构时检测到内存泄漏，应该会失败
    EXPECT_DEATH({
        delete pool;
    }, "Memory leak detected!");
    
    // 清理以避免实际的内存泄漏
    pool->destroy(item);
    delete pool;
}
#endif

// 测试在压力情况下的内存回收和重用
TEST(MemoryPoolTest, StressTest) {
    const int iterations = 10000;
    const int objectCount = 1000;
    
    MemoryPool<TestItem, true> pool(objectCount / 10);
    
    for (int iter = 0; iter < iterations; ++iter) {
        std::vector<TestItem*> items;
        items.reserve(objectCount);
        
        // 分配所有对象
        for (int i = 0; i < objectCount; ++i) {
            items.push_back(pool.construct(i, "stress"));
        }
        
        // 释放所有对象
        for (auto item : items) {
            pool.destroy(item);
        }
        
        // 确保内存被正确回收
        EXPECT_EQ(pool.allocated_count(), 0);
        if (iter % 100 == 0) {
            std::cout << "Stress test iteration " << iter << ", total blocks: " << pool.total_count() << std::endl;
        }
    }
}

// 主函数
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}