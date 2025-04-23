#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <functional>
#include "memoryPool.hpp"

// 示例1: 基本使用
void basicUsageExample() {
    std::cout << "\n===== 基本使用示例 =====\n";
    
    // 创建内存池 - 每个块包含100个对象
    MemoryPool<std::string> pool(100);
    
    // 获取初始状态
    std::cout << "初始空闲块: " << pool.free_count() << std::endl;
    std::cout << "总块数: " << pool.total_count() << std::endl;
    
    // 分配和构造
    std::string* str1 = pool.allocate();
    new (str1) std::string("Hello, World!");
    
    std::string* str2 = pool.construct("直接构造的字符串");
    
    // 使用对象
    std::cout << "字符串1: " << *str1 << std::endl;
    std::cout << "字符串2: " << *str2 << std::endl;
    
    // 销毁和释放
    str1->~basic_string();
    pool.deallocate(str1);
    
    pool.destroy(str2);
    
    // 检查最终状态
    std::cout << "最终空闲块: " << pool.free_count() << std::endl;
}

// 示例2: 使用自定义类
class MyClass {
public:
    MyClass() : id(0), name("默认") {
        std::cout << "MyClass默认构造: " << name << std::endl;
    }
    
    MyClass(int i, std::string n) : id(i), name(std::move(n)) {
        std::cout << "MyClass构造: id=" << id << ", name=" << name << std::endl;
    }
    
    ~MyClass() {
        std::cout << "MyClass析构: id=" << id << ", name=" << name << std::endl;
    }
    
    void print() const {
        std::cout << "MyClass(id=" << id << ", name=" << name << ")" << std::endl;
    }
    
private:
    int id;
    std::string name;
};

void customClassExample() {
    std::cout << "\n===== 自定义类示例 =====\n";
    
    // 创建内存池
    MemoryPool<MyClass> pool(10);
    
    // 使用construct创建对象
    MyClass* obj1 = pool.construct();
    MyClass* obj2 = pool.construct(42, "测试对象");
    
    // 使用对象
    obj1->print();
    obj2->print();
    
    // 销毁对象
    pool.destroy(obj1);
    pool.destroy(obj2);
}

// 示例3: 智能指针
void smartPointerExample() {
    std::cout << "\n===== 智能指针示例 =====\n";
    
    MemoryPool<MyClass> pool(10);
    
    {
        // 创建智能指针
        std::cout << "创建智能指针..." << std::endl;
        auto obj = pool.make_shared(100, "智能指针管理的对象");
        
        // 使用智能指针
        obj->print();
        
        std::cout << "智能指针离开作用域..." << std::endl;
    } // 智能指针在这里自动销毁对象并返回内存到池
    
    std::cout << "智能指针已销毁" << std::endl;
    std::cout << "空闲块: " << pool.free_count() << std::endl;
}

// 示例4: 性能对比
template<typename T>
void runPerformanceTest(const std::string& name, std::function<void(std::vector<T*>&, int)> allocFunc, 
                        std::function<void(std::vector<T*>&)> freeFunc, int iterations) {
    std::vector<T*> items;
    items.reserve(iterations);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // 分配
    allocFunc(items, iterations);
    
    auto midpoint = std::chrono::high_resolution_clock::now();
    
    // 释放
    freeFunc(items);
    
    auto end = std::chrono::high_resolution_clock::now();
    
    auto allocTime = std::chrono::duration_cast<std::chrono::microseconds>(midpoint - start).count();
    auto freeTime = std::chrono::duration_cast<std::chrono::microseconds>(end - midpoint).count();
    auto totalTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
    std::cout << name << ":\n";
    std::cout << "  分配时间: " << allocTime << " 微秒 (" << allocTime / iterations << " 微秒/项)\n";
    std::cout << "  释放时间: " << freeTime << " 微秒 (" << freeTime / iterations << " 微秒/项)\n";
    std::cout << "  总时间: " << totalTime << " 微秒\n";
}

struct LargeObject {
    char data[256];
    std::string name;
    int id;
    
    LargeObject() : id(0), name("default") {
        for (int i = 0; i < 256; ++i) data[i] = 0;
    }
    
    LargeObject(int i, const std::string& n) : id(i), name(n) {
        for (int i = 0; i < 256; ++i) data[i] = 'a' + (i % 26);
    }
};

void performanceComparisonExample() {
    std::cout << "\n===== 性能对比示例 =====\n";
    
    const int iterations = 100000;
    
    // 创建内存池
    MemoryPool<LargeObject> pool(iterations / 10);
    pool.reserve(10); // 预分配10个块
    
    std::cout << "执行 " << iterations << " 次分配和释放...\n";
    
    // 测试标准分配器
    auto stdAlloc = [](std::vector<LargeObject*>& items, int count) {
        for (int i = 0; i < count; ++i) {
            items.push_back(new LargeObject(i, "std"));
        }
    };
    
    auto stdFree = [](std::vector<LargeObject*>& items) {
        for (auto item : items) {
            delete item;
        }
        items.clear();
    };
    
    // 测试内存池
    auto poolAlloc = [&pool](std::vector<LargeObject*>& items, int count) {
        for (int i = 0; i < count; ++i) {
            items.push_back(pool.construct(i, "pool"));
        }
    };
    
    auto poolFree = [&pool](std::vector<LargeObject*>& items) {
        for (auto item : items) {
            pool.destroy(item);
        }
        items.clear();
    };
    
    // 运行测试
    runPerformanceTest<LargeObject>("标准分配器", stdAlloc, stdFree, iterations);
    runPerformanceTest<LargeObject>("内存池", poolAlloc, poolFree, iterations);
}

// 示例5: 多线程
void threadFunction(MemoryPool<LargeObject>& pool, int threadId, int iterations) {
    std::vector<LargeObject*> items;
    items.reserve(iterations);
    
    // 分配和构造
    for (int i = 0; i < iterations; ++i) {
        items.push_back(pool.construct(threadId * 1000 + i, "thread-" + std::to_string(threadId)));
        
        // 模拟一些工作
        if (i % 100 == 0) {
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
    }
    
    // 释放一半对象
    for (int i = 0; i < iterations / 2; ++i) {
        pool.destroy(items[i]);
        items[i] = nullptr;
    }
    
    // 重新分配一些对象
    for (int i = 0; i < iterations / 4; ++i) {
        items.push_back(pool.construct(threadId * 2000 + i, "realloc-" + std::to_string(threadId)));
    }
    
    // 释放所有对象
    for (auto item : items) {
        if (item) pool.destroy(item);
    }
}

void multithreadedExample() {
    std::cout << "\n===== 多线程示例 =====\n";
    
    const int threadCount = 4;
    const int iterationsPerThread = 10000;
    
    // 使用线程本地存储的内存池
    MemoryPool<LargeObject, true> pool(iterationsPerThread / 4);
    pool.reserve(threadCount * 2);
    
    // 创建线程
    std::vector<std::thread> threads;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // 启动线程
    for (int i = 0; i < threadCount; ++i) {
        threads.emplace_back(threadFunction, std::ref(pool), i, iterationsPerThread);
    }
    
    // 等待所有线程完成
    for (auto& t : threads) {
        t.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    std::cout << threadCount << " 个线程各执行 " << iterationsPerThread << " 次操作\n";
    std::cout << "总耗时: " << duration << " 毫秒\n";
    std::cout << "内存池状态:\n";
    pool.printStats();
}

// 示例6: 内存使用统计
void memoryStatsExample() {
    std::cout << "\n===== 内存使用统计示例 =====\n";
    
    // 创建内存池
    MemoryPool<MyClass> pool(10, 5); // 每块10个对象，最多5个块
    
    // 打印初始状态
    std::cout << "初始状态:\n";
    pool.printStats();
    
    // 分配一些对象
    std::vector<MyClass*> objects;
    for (int i = 0; i < 25; ++i) {
        try {
            objects.push_back(pool.construct(i, "对象" + std::to_string(i)));
        } catch (const std::bad_alloc& e) {
            std::cout << "分配失败: " << e.what() << std::endl;
            break;
        }
    }
    
    // 打印分配后状态
    std::cout << "\n分配后状态:\n";
    pool.printStats();
    
    // 释放一部分对象
    for (size_t i = 0; i < objects.size() / 2; ++i) {
        pool.destroy(objects[i]);
        objects[i] = nullptr;
    }
    
    // 打印部分释放后状态
    std::cout << "\n部分释放后状态:\n";
    pool.printStats();
    
    // 清理所有对象
    for (auto obj : objects) {
        if (obj) pool.destroy(obj);
    }
    
    // 打印最终状态
    std::cout << "\n最终状态:\n";
    pool.printStats();
}

// 主函数
int main() {
    std::cout << "===== C++11 内存池使用示例 =====\n";
    
    try {
        // 运行所有示例
        basicUsageExample();
        customClassExample();
        smartPointerExample();
        performanceComparisonExample();
        multithreadedExample();
        memoryStatsExample();
        
        std::cout << "\n所有示例执行完成!\n";
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}