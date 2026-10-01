#include <gtest/gtest.h>

#include <algorithm>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "struo/struo.hpp"

namespace {

struct Cameras {};
struct Nodes {};

struct Config {
    std::map<int, std::string> cameras;
    std::optional<std::map<std::string, int>> named;
    int camera{};
    std::optional<std::string> name;
};

struct EqualityKey {
    int value;
    bool operator==(const EqualityKey&) const = default;
};

} // namespace

namespace struo {

template<>
struct DomainTraits<Cameras> {
    static constexpr std::string_view name() { return "cameras"; }
};

template<>
struct DomainTraits<Nodes> {
    // Names are metadata; equal names must not collapse type identity.
    static constexpr std::string_view name() { return "cameras"; }
};

} // namespace struo

namespace {

using namespace struo;

TEST(ReferenceMetadata, UnannotatedFieldHasNoDomains) {
    const Field<&Config::camera> field{Keys{"camera"}};
    EXPECT_FALSE(field.hasDefinitions());
    EXPECT_FALSE(field.isReference());
    EXPECT_TRUE(field.getDefinitions().empty());
    EXPECT_EQ(field.getReference().domain_id_, nullptr);
    EXPECT_EQ(field.getReference().type_, nullptr);
}

TEST(ReferenceMetadata, MapPublishesDistinctDomainsOnce) {
    const Field<&Config::cameras> field{
        Keys{"cameras"}, Defines<Cameras, Nodes, Cameras>, Defines<Nodes>
    };
    ASSERT_TRUE(field.hasDefinitions());
    EXPECT_FALSE(field.isReference());
    const auto domains = field.getDefinitions();
    ASSERT_EQ(domains.size(), 2u);
    EXPECT_EQ(domains[0], detail::DomainIdOf<Cameras>);
    EXPECT_EQ(domains[1], detail::DomainIdOf<Nodes>);
    EXPECT_NE(domains[0], domains[1]);
    EXPECT_EQ(domains[0]->name, "cameras");
    EXPECT_EQ(domains[1]->name, "cameras");

    const Field<&Config::named> optional_map{Keys{"named"}, Defines<Cameras>};
    EXPECT_TRUE(optional_map.hasDefinitions());
}

TEST(ReferenceMetadata, ReferenceRecordsUnderlyingKeyType) {
    const Field<&Config::camera> integer{Keys{"camera"}, References<Cameras>};
    const Field<&Config::name> optional_string{Keys{"name"}, References<Cameras>};
    ASSERT_TRUE(integer.isReference());
    ASSERT_TRUE(optional_string.isReference());
    EXPECT_EQ(integer.getReference().domain_id_, detail::DomainIdOf<Cameras>);
    EXPECT_EQ(integer.getReference().type_, detail::InstanceTypeIdOf<int>);
    EXPECT_EQ(optional_string.getReference().type_, detail::InstanceTypeIdOf<std::string>);
    EXPECT_EQ((detail::InstanceTypeIdOf<const int&>), detail::InstanceTypeIdOf<int>);
}

TEST(ReferenceMetadata, MatchesValuesAtDifferentAddresses) {
    const std::map<std::string, int> definitions{{"front", 1}};
    const std::string reference = "front";
    const auto target = detail::make_definition_key(detail::DomainIdOf<Cameras>, definitions.begin()->first);
    const auto query = detail::make_definition_key(detail::DomainIdOf<Cameras>, reference);
    ASSERT_NE(target.addr_, query.addr_);
    EXPECT_EQ(target, query);
}

TEST(ReferenceMetadata, SupportsLinearLookupWithEqualityOnlyKeys) {
    const EqualityKey first{1}, second{2}, reference{2}, missing{3};
    const auto domain = detail::DomainIdOf<Cameras>;
    const std::vector<detail::DefinitionKey> definitions{
        detail::make_definition_key(domain, first),
        detail::make_definition_key(domain, second)
    };
    const auto found = std::ranges::find(definitions, detail::make_definition_key(domain, reference));
    ASSERT_NE(found, definitions.end());
    EXPECT_EQ(found->addr_, &second);
    EXPECT_EQ(std::ranges::find(definitions, detail::make_definition_key(domain, missing)), definitions.end());
}

TEST(ReferenceMetadata, DomainAndTypeArePartOfIdentity) {
    const int integer = 12;
    const unsigned unsigned_integer = 12;
    const std::string string = "12";
    const auto key = detail::make_definition_key(detail::DomainIdOf<Cameras>, integer);
    EXPECT_NE(key, detail::make_definition_key(detail::DomainIdOf<Nodes>, integer));
    EXPECT_NE(key, detail::make_definition_key(detail::DomainIdOf<Cameras>, unsigned_integer));
    EXPECT_NE(key, detail::make_definition_key(detail::DomainIdOf<Cameras>, string));
}

#ifndef STRUO_NO_ASSERT
TEST(ReferenceMetadata, RejectsMultipleReferenceAnnotations) {
    EXPECT_DEATH(([] {
        const Field<&Config::camera> field{
            Keys{"camera"}, References<Cameras>, References<Nodes>
        };
        (void)field;
    }()), "duplicate references");
}
#endif

} // namespace
