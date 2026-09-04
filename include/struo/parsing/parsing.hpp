#pragma once

#include <utility>

#include "struo/forward.hpp"
#include "struo/Result.hpp"
#include "struo/detail/detail.hpp"

namespace struo {

    template<typename UserObject, typename Parser>
    constexpr Result<UserObject> load(Parser&& parser) {
        auto parsed = detail::parse<UserObject>(std::forward<Parser>(parser));
        if(!parsed) {
            return parsed.error();
        }

        return detail::materialize<UserObject>(*parsed);
    }

}
