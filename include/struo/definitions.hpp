#pragma once

#include "struo/detail/debug.hpp"

#if defined(STRUO_NO_ASSERT)
    #define STRUO_ASSERT(cond, ...) ((void)0)
#else
    #define STRUO_ASSERT(cond, ...) do { \
        if(!(cond)) [[unlikely]] { \
            ::struo::detail::print_to_stderr_and_abort( \
                std::source_location::current(), \
                #cond __VA_OPT__(,) __VA_ARGS__); \
        } \
    } while (false)
#endif // STRUO_NO_ASSERT
