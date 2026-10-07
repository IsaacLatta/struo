#include <deque>
#include <list>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <gtest/gtest.h>

#include "struo/struo.hpp"

using namespace struo;

namespace {

template<typename Container>
Container makeContainer(std::size_t size) {
    Container container{};

    for(std::size_t i = 0; i < size; ++i) {
        if constexpr(IsMap<Container>) {
            container.emplace(static_cast<typename Container::key_type>(i), static_cast<typename Container::mapped_type>(i));
        } else if constexpr(IsSequence<Container>) {
            container.emplace_back(static_cast<typename Container::value_type>(i));
        } else if constexpr(IsScalar<Container>) {
            static_assert(std::same_as<Container, std::string>);
            container.push_back(static_cast<char>('a' + i));
        } else {
            container.emplace(static_cast<typename Container::value_type>(i));
        }
    }

    return container;
}

template<typename T>
class SizeConstraintTest : public ::testing::Test {};

using ContainerTypes = ::testing::Types<
    std::vector<int>,
    std::deque<int>,
    std::list<int>,
    std::set<int>,
    std::unordered_set<int>,
    std::map<int, int>,
    std::unordered_map<int, int>,
    std::string
>;

TYPED_TEST_SUITE(SizeConstraintTest, ContainerTypes);

TYPED_TEST(SizeConstraintTest, SizeRangeIsInclusive) {
    const auto container = makeContainer<TypeParam>(3);

    EXPECT_TRUE((SizeRange<2, 4>(container)));
    EXPECT_TRUE((SizeRange<3, 3>(container)));
    EXPECT_FALSE((SizeRange<0, 2>(container)));
    EXPECT_FALSE((SizeRange<4, 8>(container)));
}

TYPED_TEST(SizeConstraintTest, SizeAtLeastIsInclusive) {
    const auto container = makeContainer<TypeParam>(3);

    EXPECT_TRUE(SizeAtLeast<2>(container));
    EXPECT_TRUE(SizeAtLeast<3>(container));
    EXPECT_FALSE(SizeAtLeast<4>(container));
}

TYPED_TEST(SizeConstraintTest, SizeAtMostIsInclusive) {
    const auto container = makeContainer<TypeParam>(3);

    EXPECT_FALSE(SizeAtMost<2>(container));
    EXPECT_TRUE(SizeAtMost<3>(container));
    EXPECT_TRUE(SizeAtMost<4>(container));
}

TYPED_TEST(SizeConstraintTest, SizeExactlyRequiresExactSize) {
    const auto container = makeContainer<TypeParam>(3);

    EXPECT_FALSE(SizeExactly<2>(container));
    EXPECT_TRUE(SizeExactly<3>(container));
    EXPECT_FALSE(SizeExactly<4>(container));
}

TYPED_TEST(SizeConstraintTest, NotEmptyRejectsEmptyContainers) {
    const TypeParam empty{};
    const auto non_empty = makeContainer<TypeParam>(1);

    EXPECT_FALSE(NotEmpty(empty));
    EXPECT_TRUE(NotEmpty(non_empty));
}

TYPED_TEST(SizeConstraintTest, FailureReturnsOutOfRangeError) {
    const auto container = makeContainer<TypeParam>(3);
    auto result = SizeExactly<4>(container);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), ARGUMENT_OUT_OF_RANGE);
}

} // namespace
