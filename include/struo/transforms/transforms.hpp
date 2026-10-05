#pragma once

#include <cctype>
#include <cstring>
#include <ios>
#include <string>
#include <filesystem>

#include "struo/Result.hpp"

#include "struo/detail/types.hpp"

namespace struo {

template<char Char>
struct TrimT {
    [[nodiscard]] std::string operator()(const std::string& str) const {
        return doTrim(str);
    }

    [[nodiscard]] std::filesystem::path operator()(const std::filesystem::path& path) const {
        return std::filesystem::path{doTrim(path.string())};
    }

private:
    [[nodiscard]] static std::string doTrim(const std::string& str) {
        auto copy { str };
        copy.erase(copy.begin(), std::find_if(copy.begin(), copy.end(), [](unsigned char ch) { return ch != Char; }));
        copy.erase(std::find_if(copy.rbegin(), copy.rend(), [](unsigned char ch) { return ch != Char; }).base(), copy.end());
        return copy;
    }
};

template<Str Path>
struct RelativeToT {
    [[nodiscard]] std::string operator()(const std::string string) const {
        return makeRelative(std::filesystem::path{string}).string();
    }

    [[nodiscard]] std::filesystem::path operator()(const std::filesystem::path& path) const {
        return makeRelative(path);
    }

private:
    [[nodiscard]] std::filesystem::path makeRelative(const std::filesystem::path& path) const {
        return std::filesystem::path { Path.string } / path;
    }
};

struct ToLowerT {
    [[nodiscard]] std::string operator()(const std::string& string) const {
        return doToLower(string);
    }

    [[nodiscard]] std::filesystem::path operator()(const std::filesystem::path& path) const {
        return doToLower(path.string());
    }

private:
    [[nodiscard]] static std::string doToLower(const std::string& string) {
        auto copy { string };
        std::transform(copy.begin(), copy.end(), copy.begin(), [](unsigned char c) { return std::tolower(c); });
        return copy;
    }
};

struct ToUpperT {
    [[nodiscard]] std::string operator()(const std::string& string) const {
        return doToUpper(string);
    }

    [[nodiscard]] std::filesystem::path operator()(const std::filesystem::path& path) const {
        return doToUpper(path.string());
    }

private:
    [[nodiscard]] static std::string doToUpper(const std::string& string) {
        auto copy { string };
        std::transform(copy.begin(), copy.end(), copy.begin(), [](unsigned char c) { return std::toupper(c); });
        return copy;
    }
};

template<Str Prefix>
struct AddPrefixT {
    [[nodiscard]] constexpr std::string operator()(const std::string& string) const {
        if(string.empty()) {
            return string;
        }

        if(string.starts_with(Prefix.string)) {
            return string;
        }

        auto copy { string };
        copy.insert(size_t{0}, Prefix.string);
        return copy;
    }
};

template<char Char>
inline constexpr TrimT<Char> Trim{};

inline constexpr auto TrimWhitespace { Trim<' '> };

template<Str Prefix>
inline constexpr AddPrefixT<Prefix> AddPrefix{};

inline constexpr auto AddLeadingSlash { AddPrefix<Str{"/"}> };

inline constexpr ToUpperT ToUpper{};

inline constexpr ToLowerT ToLower{};

template<Str ParentDir>
inline constexpr RelativeToT<ParentDir> RelativeTo{};

}
