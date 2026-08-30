#pragma once

#include <string_view>

#include <magic_enum/magic_enum.hpp>
#include <yaml-cpp/yaml.h>

namespace struo {

    enum struct FileFormat {
        AUTO,
        YAML,
    };

     [[nodiscard]] constexpr std::string_view get_project_version() {
        return STRUO_VERSION;
    }

    [[nodiscard]] constexpr bool is_debug_enabled() {
         return static_cast<bool>(STRUO_DEBUG);
     }

    template<typename T>
    requires std::is_enum_v<T>
    [[nodiscard]] constexpr std::string_view get_enum_name(T t) {
         return magic_enum::enum_name<T>(t);
    }

    inline bool is_yaml_linked() {
         YAML::Node node{};
         return true;
     }
}