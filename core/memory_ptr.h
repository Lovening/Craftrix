#ifndef __MEMORY_PTR_H__
#define __MEMORY_PTR_H__
#include <memory>

// 判断C++标准版本，如果小于C++14则自定义make_shared
#if !defined(__cplusplus) || (__cplusplus < 201402L)

// 针对主流平台的C++编译器（GCC, Clang, MSVC）
// MSVC的__cplusplus在较老版本可能不是标准值，但C++14支持也较新
#if defined(_MSC_VER)
    #if _MSC_VER < 1900 // VS2015及以上才支持C++14
        #define NEED_CUSTOM_MAKE_SHARED
    #endif
#else
    #define NEED_CUSTOM_MAKE_SHARED
#endif

#endif

#ifdef NEED_CUSTOM_MAKE_SHARED
namespace std
{
	template<typename T>
	inline std::shared_ptr<T> make_shared()
	{
		return std::shared_ptr<T>(new T());
	}

	template<typename T, typename Arg1>
	inline std::shared_ptr<T> make_shared(Arg1 arg1)
	{
		return std::shared_ptr<T>(new T(arg1));
	}

	template<typename T, typename Arg1, typename Arg2>
	inline std::shared_ptr<T> make_shared(Arg1 arg1, Arg2 arg2)
	{
		return std::shared_ptr<T>(new T(arg1, arg2));
	}

	template<typename T, typename Arg1, typename Arg2, typename Arg3>
	inline std::shared_ptr<T> make_shared(Arg1 arg1, Arg2 arg2, Arg3 arg3)
	{
		return std::shared_ptr<T>(new T(arg1, arg2, arg3));
	}

	template<typename T, typename Arg1, typename Arg2, typename Arg3, typename Arg4>
	inline std::shared_ptr<T> make_shared(Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4)
	{
		return std::shared_ptr<T>(new T(arg1, arg2, arg3, arg4));
	}

	template<typename T, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
	inline std::shared_ptr<T> make_shared(Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5)
	{
		return std::shared_ptr<T>(new T(arg1, arg2, arg3, arg4, arg5));
	}

	template<typename T, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6>
	inline std::shared_ptr<T> make_shared(Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6)
	{
		return std::shared_ptr<T>(new T(arg1, arg2, arg3, arg4, arg5, arg6));
	}


    // make_unique
    template<typename T>
    inline std::unique_ptr<T> make_unique()
    {
        return std::unique_ptr<T>(new T());
    }

    template<typename T, typename Arg1>
    inline std::unique_ptr<T> make_unique(Arg1 arg1)
    {
        return std::unique_ptr<T>(new T(arg1));
    }

    template<typename T, typename Arg1, typename Arg2>
    inline std::unique_ptr<T> make_unique(Arg1 arg1, Arg2 arg2)
    {
        return std::unique_ptr<T>(new T(arg1, arg2));
    }

    template<typename T, typename Arg1, typename Arg2, typename Arg3>
    inline std::unique_ptr<T> make_unique(Arg1 arg1, Arg2 arg2, Arg3 arg3)
    {
        return std::unique_ptr<T>(new T(arg1, arg2, arg3));
    }

    template<typename T, typename Arg1, typename Arg2, typename Arg3, typename Arg4>
    inline std::unique_ptr<T> make_unique(Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4)
    {
        return std::unique_ptr<T>(new T(arg1, arg2, arg3, arg4));
    }

    template<typename T, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
    inline std::unique_ptr<T> make_unique(Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5)
    {
        return std::unique_ptr<T>(new T(arg1, arg2, arg3, arg4, arg5));
    }

    template<typename T, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6>
    inline std::unique_ptr<T> make_unique(Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6)
    {
        return std::unique_ptr<T>(new T(arg1, arg2, arg3, arg4, arg5, arg6));
    }

}
#endif // NEED_CUSTOM_MAKE_SHARED
#endif // __MEMORY_PTR_H__