#pragma once

#include <string>

#include "struo/Result.hpp"

namespace struo::detail {

#if STRUO_PLATFORM_LINUX

    Result<void> is_valid_ipv4(const std::string&);

    Result<void> is_valid_ipv6(const std::string&);

    Result<void> is_valid_hostname(const std::string&);

#endif // STRUO_PLATFORM_LINUX

}
