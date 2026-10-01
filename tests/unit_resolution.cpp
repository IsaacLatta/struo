#include <gtest/gtest.h>

#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "struo/struo.hpp"

namespace {

struct ResolutionCameras {};
using CameraMap = std::map<std::string, int>;

struct CameraUse {
    std::optional<std::string> camera;
    std::optional<int> numeric;
};

using ResolutionChoice = std::variant<int, CameraUse>;
enum class ReferenceKind { NUMBER, USE };

struct ResolutionBranch {
    std::vector<CameraUse> children;
    std::map<std::string, CameraUse> named;
    ResolutionChoice choice;
    std::optional<CameraMap> cameras;
};

struct ResolutionConfig {
    std::optional<std::string> camera;
    std::vector<ResolutionBranch> branches;
    CameraMap cameras;
    CameraMap extra;
};

} // namespace

namespace struo {

template<>
struct DomainTraits<ResolutionCameras> {
    static constexpr std::string_view name() { return "cameras"; }
};

template<>
struct SchemaTraits<CameraUse> {
    static auto schema() {
        return Object{
            Field<&CameraUse::camera>{Keys{"camera"}, References<ResolutionCameras>},
            Field<&CameraUse::numeric>{Keys{"numeric"}, References<ResolutionCameras>}
        };
    }
};

template<>
struct SchemaTraits<ResolutionChoice> {
    static auto schema() {
        return Variant{Bindings{Bind<ReferenceKind::USE, CameraUse>{}, Bind<ReferenceKind::NUMBER, int>{}}};
    }
};

template<>
struct SchemaTraits<ResolutionBranch> {
    static auto schema() {
        return Object{
            Field<&ResolutionBranch::children>{Keys{"children"}},
            Field<&ResolutionBranch::named>{Keys{"named"}},
            Field<&ResolutionBranch::choice>{Keys{"choice"}},
            Field<&ResolutionBranch::cameras>{Keys{"cameras"}, Defines<ResolutionCameras>}
        };
    }
};

template<>
struct SchemaTraits<ResolutionConfig> {
    static auto schema() {
        return Object{
            Field<&ResolutionConfig::camera>{Keys{"camera", "selected"}, References<ResolutionCameras>,
                Defaults{[] { return std::optional<std::string>{"fallback"}; }}},
            Field<&ResolutionConfig::branches>{Keys{"branches"}},
            Field<&ResolutionConfig::cameras>{Keys{"cameras"}, Defines<ResolutionCameras>,
                Defaults{[] { return CameraMap{{"fallback", 1}}; }}},
            Field<&ResolutionConfig::extra>{Keys{"extra"}, Defines<ResolutionCameras>}
        };
    }
};

} // namespace struo

namespace {

auto load_config(std::string_view yaml) {
    return struo::load<ResolutionConfig>(struo::YamlParser{YAML::Load(std::string{yaml})});
}

void expect_unresolved(std::string_view yaml, std::string_view path) {
    const auto result = load_config(yaml);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), struo::KEY_NOT_FOUND);
    EXPECT_EQ(result.error().what(), std::string{path} + ": unresolved reference in domain \"cameras\"");
}

TEST(Resolution, ResolvesSiblingAndGrandparentDefinitionsWithOuterFallback) {
    const auto result = load_config(R"(
camera: outer
cameras: {outer: 1}
branches:
  - cameras: {inner: 2}
    children: [{camera: inner}, {camera: outer}]
    named: {main: {camera: outer}}
    choice: {type: USE, value: {camera: inner}}
)");
    ASSERT_TRUE(result) << result.error().what();
    EXPECT_EQ(result.value().branches[0].children[1].camera, "outer");
}

TEST(Resolution, SearchesMultipleMapsInTheSameScope) {
    const auto result = load_config("camera: second\ncameras: {first: 1}\nextra: {second: 2}");
    ASSERT_TRUE(result) << result.error().what();
}

TEST(Resolution, ValidatesDefaultsAndReportsMissingDefaultReferences) {
    const auto result = load_config("{}");
    ASSERT_TRUE(result) << result.error().what();
    EXPECT_EQ(result.value().camera, "fallback");
    EXPECT_TRUE(result.value().cameras.contains("fallback"));
    expect_unresolved("cameras: {}", "camera");
    expect_unresolved("selected: missing", "selected");
}

TEST(Resolution, DoesNotExposeDefinitionsToAncestorsOrSiblingSubtrees) {
    expect_unresolved("camera: local\nbranches: [{cameras: {local: 1}}]", "camera");
    expect_unresolved(R"(
branches:
  - cameras: {local: 1}
    children: [{camera: local}]
  - children: [{camera: local}]
)", "branches[1].children[0].camera");
}

TEST(Resolution, RequiresMatchingKeyTypesAndTracksContainerPaths) {
    expect_unresolved("branches: [{cameras: {'1': 1}, children: [{numeric: 1}]}]",
        "branches[0].children[0].numeric");
    expect_unresolved("branches: [{named: {main: {camera: missing}}}]",
        "branches[0].named[\"main\"].camera");
    expect_unresolved("branches: [{choice: {type: USE, value: {camera: missing}}}]",
        "branches[0].choice<USE>.value.camera");
}

} // namespace
