#pragma once

#include <string>

#include "struo/Result.hpp"

namespace struo::detail {

#if STRUO_PLATFORM_LINUX

    Result<void> is_valid_ipv4(const std::string&);

    Result<void> is_valid_ipv6(const std::string&);

    Result<void> is_valid_hostname(const std::string&);

    void set_env_variable(const std::string& key, const std::string& value) noexcept;

    void unset_env_variable(const std::string& key, const std::string& value) noexcept;

#endif // STRUO_PLATFORM_LINUX

}
