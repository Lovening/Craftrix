#ifdef __PUBVARANDSTRUCT_H__
#define __PUBVARANDSTRUCT_H__

#if __cplusplus >= 201703L  // C++17 或更高
    // C++17 特性代码
#define NODISCARD [[nodiscard]]
#elif __cplusplus >= 201402L  // C++14
    // C++14 特性代码
#define NODISCARD
#elif __cplusplus >= 201103L  // C++11
    // C++11 特性代码
#define NODISCARD
#else
    // 较老的 C++ 标准
#endif

#endif