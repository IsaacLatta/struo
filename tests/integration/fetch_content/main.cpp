#include <struo/struo.hpp>

int main() {
    struo::Result<void> result { struo::ok() };
    if(!result) {
        return 1;
    }

    return 0;
}