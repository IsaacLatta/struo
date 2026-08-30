#pragma once

namespace struo {

    template<typename T>
    class Field {
    public:
        template<typename... Args>
        explicit Field(Args&&... args) {

        }

    private:
        void apply(T&& t) {
            data_t = T{ std::forward<T>(t) };
        }

    private:
        T data_;

    };

}