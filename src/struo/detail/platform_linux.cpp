#if STRUO_PLATFORM_LINUX

#include "struo/detail/platform.hpp"
#include "struo/Error.hpp"
#include "struo/Result.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <resolv.h>

namespace struo::detail {
    namespace {
        Result<void> classify_inet_result(int code, const std::string& address, std::string_view family) {
            switch (code) {
            case 1:
                return ok();
            case 0:
                return err(INVALID_ARGUMENT, std::format("address \"{}\" is not a valid {} address", address, family));
            case -1:
            default:
                return err(UNKNOWN_ERROR, std::format("failed to validate {} address \"{}\"", family, address));
            }
        }
    }

    Result<void> is_valid_ipv4(const std::string& address) {
        ::in_addr addr{};
        return classify_inet_result(::inet_pton(AF_INET, address.c_str(), &addr), address, "ipv4");
    }

    Result<void> is_valid_ipv6(const std::string& address) {
        ::in6_addr addr{};
        return classify_inet_result(::inet_pton(AF_INET6, address.c_str(), &addr), address, "ipv6");
    }

    Result<void> is_valid_hostname(const std::string& hostname) {
        if(::res_hnok(hostname.c_str()) == 1) {
            return ok();
        }

        return err(INVALID_ARGUMENT, std::format("host name \"{}\" is not valid", hostname));
    }

    void set_env_variable(const std::string& key, const std::string& value) noexcept {
        static constexpr int replace_if_present { 1 };
        (void)::setenv(key.c_str(), value.c_str(), replace_if_present);
    }

    void unset_env_variable(const std::string& key, const std::string& value) noexcept {
        (void)::unsetenv(key.c_str());
    }
}

#endif
