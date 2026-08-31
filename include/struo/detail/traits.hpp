#pragma once

#include <concepts>
#include <type_traits>

#include "struo/concepts.hpp"

namespace struo::detail {

    template<typename Object, typename Value>
    struct MemberTraits<Value Object::*> {
        using value_type = Value;
        using object_type = Object;
    };

    template<typename T>
    struct ValueTraits;

    template<typename T>
    requires IsMap<T>
    struct ValueTraits<T> {
        using key_type = typename T::key_type;
        using mapped_type = typename T::mapped_type;
    };

    template<typename T>
    requires IsSequence<T>
    struct ValueTraits<T> {
        using element_type = typename T::value_type;
    };

    template<typename T>
    requires IsScalar<T>
    struct ValueTraits<T> {
        using value_type = T;
    };

    template<typename T>
    requires IsObject<T>
    struct ValueTraits<T> {
        using value_type = T;
        using schema_type = decltype(SchemaTraits<T>::schema());
    };
}