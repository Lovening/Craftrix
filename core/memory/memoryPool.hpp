#ifndef _MEMORY_POOL_H_
#define _MEMORY_POOL_H_

#include <iostream>
#include <vector>
#include <mutex>
#include <memory>
#include <new>
#include <cassert>
#include <type_traits>
#include <algorithm>
#include <thread>
#include <unordered_map>

/**
 * @brief 高性能内存池实现，支持C++11
 * 
 * 此内存池用于高效分配和回收固定大小对象的内存，特别适用于
 * 频繁创建和销毁相同类型对象的场景，如游戏引擎、网络服务器等。
 * 
 * @tparam T 要分配的对象类型
 * @tparam ThreadLocal 是否使用线程本地存储提高并发性能(默认开启)
 */
template <typename T, bool ThreadLocal = true>
class MemoryPool {
public:
    /**
     * @brief 构造函数
     * @param chunkBlockCount 每个内存块中包含的对象数量
     * @param maxChunks 最大内存块数量(0表示无限制)
     */
    explicit MemoryPool(size_t chunkBlockCount = 1024, size_t maxChunks = 0);
    
    /**
     * @brief 析构函数
     * 会检查内存泄漏并释放所有分配的内存
     */
    ~MemoryPool();

    // 禁止复制
    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

    /**
     * @brief 分配一个原始内存块
     * @return 指向分配内存的指针
     * @throw std::bad_alloc 如果内存分配失败
     */
    T* allocate();

    /**
     * @brief 分配内存并构造对象
     * @param args 传递给对象构造函数的参数
     * @return 指向新构造对象的指针
     * @throw 可能抛出构造函数中的异常，已做异常安全处理
     */
    template<typename... Args>
    T* construct(Args&&... args);
    
    /**
     * @brief 释放一个先前分配的内存块
     * @param ptr 要释放的内存指针
     */
    void deallocate(T* ptr);
    
    /**
     * @brief 调用对象析构函数并释放内存
     * @param ptr 要销毁的对象指针
     */
    void destroy(T* ptr);

    /**
     * @brief 获取智能指针包装
     * 返回自定义智能指针，自动管理内存池对象的生命周期
     * @param args 传递给对象构造函数的参数
     * @return 智能指针
     */
    template<typename... Args>
    std::shared_ptr<T> make_shared(Args&&... args) {
        T* ptr = construct(std::forward<Args>(args)...);
        return std::shared_ptr<T>(ptr, [this](T* p) { this->destroy(p); });
    }

    /**
     * @brief 获取剩余可用块数量
     * @return 当前可用的内存块数量
     */
    size_t free_count() const;
    
    /**
     * @brief 获取总共分配的块数量
     * @return 总内存块数量
     */
    size_t total_count() const;
    
    /**
     * @brief 获取已分配的块数量
     * @return 已分配的内存块数量
     */
    size_t allocated_count() const;
    
    /**
     * @brief 打印内存池使用统计信息
     * @param os 输出流
     */
    void printStats(std::ostream& os = std::cout) const;

    /**
     * @brief 预分配内存块
     * @param numChunks 要预分配的内存块数量
     */
    void reserve(size_t numChunks);

private:
    // 内存块链表节点
    struct FreeChunk {
        FreeChunk* next;
    };

    // 线程本地存储结构
    struct ThreadCache {
        FreeChunk* freeList;
        size_t freeCount;

        ThreadCache() : freeList(nullptr), freeCount(0) {}
    };

    // 分配一个新的内存块
    void allocateChunk();
    
    // 获取当前线程的缓存
    ThreadCache& getThreadCache() const;

    // 从全局池获取一些块到线程缓存
    void refillThreadCache(ThreadCache& cache);

    // 返回块到全局池
    void returnToGlobalPool(FreeChunk* chunk, size_t count);

        // 辅助函数：计算最大尺寸
    static constexpr size_t getMaxSize() {
        return sizeof(T) > sizeof(FreeChunk) ? sizeof(T) : sizeof(FreeChunk);
    }
    // 辅助函数：计算最大对齐
    static constexpr size_t getMaxAlign() {
        return alignof(T) > alignof(FreeChunk) ? alignof(T) : alignof(FreeChunk);
    }

    // 计算对齐后的块大小
    static constexpr size_t calcAlignedSize() {
    /**
     * 代码 (maxSize + maxAlign - 1) & ~(maxAlign - 1) 是一个常用的计算向上对齐 (round up to the nearest multiple of alignment) 的技巧。
     * maxAlign 是所需的对齐值（必须是 2 的幂）。
     * maxAlign - 1 会得到一个二进制全是 1 的掩码（例如，对齐 8，掩码是 7 或 0b111）。
     * ~(maxAlign - 1) 会得到一个清除低位的掩码（例如，对齐 8，掩码是 ...11111000）。
     * maxSize + maxAlign - 1 确保即使 maxSize 已经是 maxAlign 的倍数，结果也能正确处理。如果 maxSize 不是倍数，加上 maxAlign - 1 会使其跨越到下一个对齐边界。
     * 最后用 & 操作符和清除低位的掩码 ~(maxAlign - 1)，将结果向下舍入到最近的 maxAlign 的倍数，实际上就是完成了向上对齐。
     */
        return (getMaxSize() + getMaxAlign() - 1) & ~(getMaxAlign() - 1);
    }

    // 验证指针是否属于内存池
    bool validatePointer(T* ptr) const;
    
    // 用于填充释放后的内存块(仅调试模式)
    void fillDeadPattern(void* ptr) const;
    
    // 检查内存块是否被释放后使用(仅调试模式)
    bool checkDeadPattern(void* ptr) const;

    // 自定义对齐内存分配函数 (C++11 兼容)
    void* allocateAligned(size_t size, size_t alignment);
    
    // 自定义对齐内存释放函数 (C++11 兼容)
    void deallocateAligned(void* ptr);

    // 内存池配置参数
    const size_t m_blockCount;    // 每个块中的对象数
    const size_t m_blockSize;     // 对齐后的块大小
    const size_t m_maxChunks;     // 最大内存块数
    const size_t m_alignment;     // 对齐要求

    // 全局内存池资源
    std::vector<void*> m_chunks;          // 已分配的内存块
    FreeChunk* m_globalFreeList;          // 全局空闲列表
    size_t m_globalFreeCount;             // 全局空闲块数量
    size_t m_total;                       // 总块数
    mutable std::mutex m_mutex;           // 全局资源锁

    // 线程本地缓存
    using TLSCacheMap = std::unordered_map<std::thread::id, ThreadCache>;
    mutable TLSCacheMap m_threadCaches;
    mutable std::mutex m_cachesMutex;

    // 调试用：记录已分配指针（仅在Debug模式启用）
#ifndef NDEBUG
    static constexpr size_t DEAD_PATTERN = 0xDEADBEEF;
    mutable std::mutex m_debugMutex;
    std::vector<T*> m_allocatedPtrs;
#endif
};

// ---------- 实现 ----------
template <typename T, bool ThreadLocal>
MemoryPool<T, ThreadLocal>::MemoryPool(size_t chunkBlockCount, size_t maxChunks)
    : m_blockCount(chunkBlockCount),
      m_blockSize(calcAlignedSize()),
      m_maxChunks(maxChunks),
      m_alignment(std::max(alignof(T), alignof(FreeChunk))),
      m_globalFreeList(nullptr),
      m_globalFreeCount(0),
      m_total(0)
{
    // 类型安全检查
    static_assert(std::is_destructible<T>::value, "T must be destructible");
    
    // 分配初始内存块
    allocateChunk();
}

template <typename T, bool ThreadLocal>
MemoryPool<T, ThreadLocal>::~MemoryPool() {

#ifndef NDEBUG
    std::lock_guard<std::mutex> debugLock(m_debugMutex);
    if (!m_allocatedPtrs.empty()) {
        std::cerr << "Memory leak detected! " << m_allocatedPtrs.size() 
                  << " objects not deallocated." << std::endl;
        assert(false && "Memory leak detected!");
        // 自动清理未释放的对象
        // for (auto ptr : m_allocatedPtrs) {
        //             // 调用析构函数但不从已分配列表中移除
        //     ptr->~T();
        // }
        // // 清空已分配列表
        // m_allocatedPtrs.clear();
        // std::cerr << "Automatic cleanup completed." << std::endl;
    }
#endif

    {
        std::lock_guard<std::mutex> cacheLock(m_cachesMutex);
        m_threadCaches.clear();
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    for (void* chunk : m_chunks) {
        deallocateAligned(chunk);
    }
}

// C++11 兼容的对齐内存分配函数
template <typename T, bool ThreadLocal>
void* MemoryPool<T, ThreadLocal>::allocateAligned(size_t size, size_t alignment) {
    // 计算需要的额外空间用于存储指针和实现对齐
    size_t headerSize = sizeof(void*);
    size_t alignmentPadding = alignment - 1;
    size_t totalSize = size + headerSize + alignmentPadding;
    
    // 分配额外的内存以便我们可以手动对齐
    void* rawMemory = ::operator new(totalSize);
    
    // 计算对齐后的内存地址
    uintptr_t rawAddress = reinterpret_cast<uintptr_t>(rawMemory);
    uintptr_t headerAddress = rawAddress + headerSize;
    uintptr_t alignedAddress = (headerAddress + alignmentPadding) & ~(alignment - 1);
    
    // 存储原始指针，以便我们可以在释放时找到它
    void** headerPtr = reinterpret_cast<void**>(alignedAddress - headerSize);
    *headerPtr = rawMemory;
    
    return reinterpret_cast<void*>(alignedAddress);
}

// C++11 兼容的对齐内存释放函数
template <typename T, bool ThreadLocal>
void MemoryPool<T, ThreadLocal>::deallocateAligned(void* ptr) {
    if (!ptr) return;
    
    // 获取原始指针并释放
    void** headerPtr = reinterpret_cast<void**>(static_cast<char*>(ptr) - sizeof(void*));
    void* rawMemory = *headerPtr;
    ::operator delete(rawMemory);
}

template <typename T, bool ThreadLocal>
void MemoryPool<T, ThreadLocal>::allocateChunk() {
    if (m_maxChunks > 0 && m_chunks.size() >= m_maxChunks) {
        throw std::bad_alloc();
    }

    size_t chunkBytes = m_blockCount * m_blockSize;
    void* mem = nullptr;

    // 尝试分配，失败时释放一些现有内存重试
    for (int retry = 0; retry < 3; ++retry) {
        try {
            mem = allocateAligned(chunkBytes, m_alignment);
            break;
        } catch (const std::bad_alloc&) {
            if (m_chunks.empty()) throw; // 无内存可释放，直接抛出
            
            // 释放最多1/4的现有内存块
            size_t toRelease = std::max(size_t(1), m_chunks.size() / 4);
            for (size_t i = 0; i < toRelease && !m_chunks.empty(); ++i) {
                deallocateAligned(m_chunks.back());
                m_chunks.pop_back();
            }
        }
    }
    if (!mem) throw std::bad_alloc();

    m_chunks.push_back(mem);
    char* start = static_cast<char*>(mem);
    
    // 将新内存块分割成空闲块链表
    FreeChunk* newList = nullptr;
    size_t newCount = 0;
    
    for (size_t i = 0; i < m_blockCount; ++i) {
        auto* c = reinterpret_cast<FreeChunk*>(start + i * m_blockSize);
        c->next = newList;
        newList = c;
        newCount++;
    }
    
    // 添加到全局空闲列表
    if (newList) {
        FreeChunk* tail = newList;
        while (tail->next) tail = tail->next;
        tail->next = m_globalFreeList;
        m_globalFreeList = newList;
        m_globalFreeCount += newCount;
    }
    
    m_total += m_blockCount;
}

template <typename T, bool ThreadLocal>
typename MemoryPool<T, ThreadLocal>::ThreadCache& 
MemoryPool<T, ThreadLocal>::getThreadCache() const {
    if (!ThreadLocal) {
        static ThreadCache dummyCache;
        return dummyCache;
    }
    
    std::thread::id tid = std::this_thread::get_id();
    
    std::lock_guard<std::mutex> lock(m_cachesMutex);
    return m_threadCaches[tid];
}

template <typename T, bool ThreadLocal>
void MemoryPool<T, ThreadLocal>::refillThreadCache(ThreadCache& cache) {
    const size_t batchSize = std::min(size_t(32), m_blockCount / 4);
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_globalFreeList) {
        allocateChunk();
    }
    
    if (!m_globalFreeList) {
        return; // 内存分配失败
    }
    
    // 从全局列表取出一批块
    FreeChunk* batchStart = m_globalFreeList;
    FreeChunk* current = batchStart;
    size_t count = 1;
    
    while (current->next && count < batchSize) {
        current = current->next;
        count++;
    }
    
    m_globalFreeList = current->next;
    current->next = cache.freeList;
    cache.freeList = batchStart;
    
    m_globalFreeCount -= count;
    cache.freeCount += count;
}

template <typename T, bool ThreadLocal>
void MemoryPool<T, ThreadLocal>::returnToGlobalPool(FreeChunk* chunk, size_t count) {
    if (!chunk) return;
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // 找到链表尾部
    FreeChunk* tail = chunk;
    size_t actualCount = 1;
    
    while (tail->next) {
        tail = tail->next;
        actualCount++;
    }
    
    // 添加到全局空闲列表
    tail->next = m_globalFreeList;
    m_globalFreeList = chunk;
    m_globalFreeCount += actualCount;
}

template <typename T, bool ThreadLocal>
T* MemoryPool<T, ThreadLocal>::allocate() {
    ThreadCache& cache = getThreadCache();
    
    // 如果线程缓存为空，从全局池获取
    if (!cache.freeList) {
        refillThreadCache(cache);
        if (!cache.freeList) {
            throw std::bad_alloc();
        }
    }
    
    // 从线程缓存分配
    FreeChunk* chunk = cache.freeList;
    cache.freeList = cache.freeList->next;
    cache.freeCount--;
    
    T* ptr = reinterpret_cast<T*>(chunk);
    
#ifndef NDEBUG
    std::lock_guard<std::mutex> debugLock(m_debugMutex);
    m_allocatedPtrs.push_back(ptr);
#endif

    return ptr;
}

template <typename T, bool ThreadLocal>
template <typename... Args>
T* MemoryPool<T, ThreadLocal>::construct(Args&&... args) {
    T* ptr = allocate();
    try {
        new (ptr) T(std::forward<Args>(args)...);
    } catch (...) {
        deallocate(ptr);
        throw;
    }
    return ptr;
}

template <typename T, bool ThreadLocal>
void MemoryPool<T, ThreadLocal>::deallocate(T* ptr) {
    if (!ptr) return;
    
#ifndef NDEBUG
    {
        std::lock_guard<std::mutex> debugLock(m_debugMutex);
        auto it = std::find(m_allocatedPtrs.begin(), m_allocatedPtrs.end(), ptr);
        if (it == m_allocatedPtrs.end()) {
            assert(false && "Deallocating invalid pointer!");
            return;
        }
        m_allocatedPtrs.erase(it);
        fillDeadPattern(ptr);
    }
#endif

    ThreadCache& cache = getThreadCache();
    
    // 将释放的块添加到线程缓存
    auto* c = reinterpret_cast<FreeChunk*>(ptr);
    c->next = cache.freeList;
    cache.freeList = c;
    cache.freeCount++;
    
    // 如果线程缓存太大，返回一些块到全局池
    const size_t maxCacheSize = m_blockCount;
    if (cache.freeCount > maxCacheSize) {
        size_t countToReturn = cache.freeCount / 2;
        FreeChunk* toReturn = cache.freeList;
        
        // 分离要返回的块
        FreeChunk* current = toReturn;
        for (size_t i = 1; i < countToReturn; ++i) {
            current = current->next;
        }
        
        cache.freeList = current->next;
        current->next = nullptr;
        cache.freeCount -= countToReturn;
        
        // 返回到全局池
        returnToGlobalPool(toReturn, countToReturn);
    }
}

template <typename T, bool ThreadLocal>
void MemoryPool<T, ThreadLocal>::destroy(T* ptr) {
    if (ptr) {
        ptr->~T();
        deallocate(ptr);
    }
}

template <typename T, bool ThreadLocal>
size_t MemoryPool<T, ThreadLocal>::free_count() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t count = m_globalFreeCount;
    
    // 统计所有线程缓存中的空闲块
    std::lock_guard<std::mutex> cacheLock(m_cachesMutex);
    for (const auto& pair : m_threadCaches) {
        count += pair.second.freeCount;
    }
    
    return count;
}

template <typename T, bool ThreadLocal>
size_t MemoryPool<T, ThreadLocal>::total_count() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_total;
}

template <typename T, bool ThreadLocal>
size_t MemoryPool<T, ThreadLocal>::allocated_count() const {
    return total_count() - free_count();
}

template <typename T, bool ThreadLocal>
void MemoryPool<T, ThreadLocal>::printStats(std::ostream& os) const {
    os << "Memory Pool Stats:" << std::endl;
    os << "  Total blocks: " << total_count() << std::endl;
    os << "  Free blocks: " << free_count() << std::endl;
    os << "  Allocated blocks: " << allocated_count() << std::endl;
    os << "  Block size: " << m_blockSize << " bytes" << std::endl;
    os << "  Alignment: " << m_alignment << " bytes" << std::endl;
    os << "  Chunks allocated: " << m_chunks.size() 
       << (m_maxChunks > 0 ? " (max: " + std::to_string(m_maxChunks) + ")" : "") << std::endl;
    os << "  Memory usage: " << (total_count() * m_blockSize) / 1024.0 << " KB" << std::endl;
    os << "  Thread local storage: " << (ThreadLocal ? "Enabled" : "Disabled") << std::endl;

#ifndef NDEBUG
    std::lock_guard<std::mutex> debugLock(m_debugMutex);
    os << "  Currently allocated objects: " << m_allocatedPtrs.size() << std::endl;
#endif
}

template <typename T, bool ThreadLocal>
void MemoryPool<T, ThreadLocal>::reserve(size_t numChunks) {
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t currentChunks = m_chunks.size();
    for (size_t i = currentChunks; i < numChunks; ++i) {
        allocateChunk();
    }
}

template <typename T, bool ThreadLocal>
bool MemoryPool<T, ThreadLocal>::validatePointer(T* ptr) const {
    if (!ptr) return false;
    
    char* charPtr = reinterpret_cast<char*>(ptr);
    std::lock_guard<std::mutex> lock(m_mutex);
    
    for (void* chunk : m_chunks) {
        char* start = static_cast<char*>(chunk);
        char* end = start + m_blockCount * m_blockSize;
        
        if (charPtr >= start && charPtr < end) {
            // 检查指针是否在块边界上
            return ((charPtr - start) % m_blockSize) == 0;
        }
    }
    
    return false;
}

#ifndef NDEBUG
template <typename T, bool ThreadLocal>
void MemoryPool<T, ThreadLocal>::fillDeadPattern(void* ptr) const {
    // 填充释放后的内存块，便于调试
    size_t* pattern = reinterpret_cast<size_t*>(ptr);
    size_t count = m_blockSize / sizeof(size_t);
    
    for (size_t i = 0; i < count; ++i) {
        pattern[i] = DEAD_PATTERN;
    }
}

template <typename T, bool ThreadLocal>
bool MemoryPool<T, ThreadLocal>::checkDeadPattern(void* ptr) const {
    size_t* pattern = reinterpret_cast<size_t*>(ptr);
    size_t count = m_blockSize / sizeof(size_t);
    
    for (size_t i = 0; i < count; ++i) {
        if (pattern[i] != DEAD_PATTERN) {
            return false;
        }
    }
    
    return true;
}
#endif


#endif // _MEMORY_POOL_H_