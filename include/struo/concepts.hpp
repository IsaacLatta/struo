#pragma once
#include <concepts>

namespace struo {

    template<typename Subject, typename... Comparators>
    concept AppearsOnce = (0 + (std::same_as<std::remove_cvref_t<Subject>, std::remove_cvref_t<Comparators>> + ...) == 1);



}
