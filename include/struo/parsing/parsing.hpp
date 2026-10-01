#pragma once

#include <utility>

#include "struo/forward.hpp"
#include "struo/Result.hpp"

#include "struo/detail/parsing_impl.hpp"
#include "struo/detail/materialization.hpp"
#include "struo/detail/resolution.hpp"

namespace struo {

    template<typename UserObject, typename Parser>
    constexpr Result<UserObject> load(Parser&& parser) {
        auto parsed = detail::parse<UserObject>(std::forward<Parser>(parser));
        if(!parsed) {
            return parsed.error();
        }

        auto object = detail::materialize<UserObject>(*parsed);
        if(!object) {
            return object.error();
        }
        if(auto resolved = detail::resolve(*object, *parsed); !resolved) {
            return resolved.error();
        }
        return object;
    }

}
