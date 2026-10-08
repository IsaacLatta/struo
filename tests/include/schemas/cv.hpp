#pragma once

#include "schemas/constraints.hpp"
#include "struo/constraints.hpp"
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <format>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace cv {

struct Point {
    std::uint64_t x{};
    std::uint64_t y{};
};

struct Resolution {
    int height{};
    int width{};
};

struct Region {
    Point origin{};
    Resolution dimensions{};
};

struct VideoCamera {
    std::filesystem::path path{};
};

struct UsbCamera {
    std::string device_id{};
    std::string preset_name{};
};

struct IpCamera {
    std::string ip{};
    std::int64_t port{};
    std::optional<std::string> username{};
    std::optional<std::string> password{};
    std::string endpoint{"/"};
};

struct GigeCamera {
    static constexpr std::uint64_t DEFAULT_MBPS{1000};

    std::string device_id{};
    std::optional<std::string> preset_name{};
    std::uint64_t mbps{DEFAULT_MBPS};
    bool compression{false};
    std::optional<Region> crop{};
};

enum class CameraType { VIDEO, IP, USB, GIGE };
using Camera = std::variant<VideoCamera, IpCamera, UsbCamera, GigeCamera>;

struct ClassificationModel {
    std::unordered_map<std::string, int> class_names{};
};

struct DetectionModel {
    std::unordered_map<std::string, int> class_names{};
    double confidence_threshold{};
};

struct SegmentationModel {
    std::unordered_map<std::string, int> class_names{};
    double mask_confidence_threshold{0.5};
};

enum class ModelType { CLASSIFICATION, DETECTION, SEGMENTATION };
using Model = std::variant<ClassificationModel, DetectionModel, SegmentationModel>;

struct Color {
    int red{};
    int green{};
    int blue{};
};

struct Letterbox {
    Resolution target{};
    Color padding{};
};

using Resize = Resolution;
using Roi = Region;
enum class ImageProcessorType { RESIZE, ROI, LETTERBOX };
using ImageProcessor = std::variant<Letterbox, Resize, Roi>;

struct CameraNode {
    Camera camera{};
};

struct ModelNode {
    Model model{};
    std::string image_node_name{};
};

struct ImageProcessorNode {
    std::vector<ImageProcessor> image_processors{};
    std::string image_node_name{};
};

enum class NodeType { CAMERA, MODEL, PROCESS };
using Node = std::variant<CameraNode, ModelNode, ImageProcessorNode>;
using NodeMap = std::map<std::string, Node>;

// A user-defined field constraint: the whole materialized map is available,
// so references can name nodes declared before or after the referring node.
struct NodeReferencesExist {
    struo::Result<void> operator()(const NodeMap& nodes) const {
        for (const auto& [name, node] : nodes) {
            const std::string* input = nullptr;
            if (const auto* model = std::get_if<ModelNode>(&node)) {
                input = &model->image_node_name;
            } else if (const auto* processor = std::get_if<ImageProcessorNode>(&node)) {
                input = &processor->image_node_name;
            }
            if (input && !nodes.contains(*input)) {
                return struo::err(struo::INVALID_VALUE,
                    std::format("node \"{}\" references missing image node \"{}\"", name, *input));
            }
        }
        return struo::ok();
    }
};

struct PeriodicTrigger {
    std::chrono::milliseconds scan_every{};
};

struct SerialTrigger {
    std::filesystem::path device{};
    std::uint32_t baud_rate{};
    std::string message{};
};

struct HttpTrigger {
    std::string path{};
};

enum class TriggerType { PERIODIC, SERIAL, HTTP };
using InspectionTrigger = std::variant<PeriodicTrigger, SerialTrigger, HttpTrigger>;

struct CommandServer {
    std::string ip{};
    std::uint16_t port{};
    std::optional<std::string> password{};
};

struct Pipeline {
    NodeMap nodes{};
    InspectionTrigger trigger{};
};

struct Config {
    std::map<std::string, Pipeline> pipelines{};
    std::optional<CommandServer> command_server{};
};

} // namespace cv

namespace struo {

template<>
struct SchemaTraits<cv::Point> {
    static auto schema() {
        return Object{
            Field<&cv::Point::x>{Keys{"x"}, REQUIRED},
            Field<&cv::Point::y>{Keys{"y"}, REQUIRED}
        };
    }
};

template<>
struct SchemaTraits<cv::Resolution> {
    static auto schema() {
        return Object{
            Field<&cv::Resolution::height>{Keys{"height"}, REQUIRED, Constraints{Positive}},
            Field<&cv::Resolution::width>{Keys{"width"}, REQUIRED, Constraints{Positive}}
        };
    }
};

template<>
struct SchemaTraits<cv::Region> {
    static auto schema() {
        return Object{
            Field<&cv::Region::origin>{Keys{"origin"}, REQUIRED},
            Field<&cv::Region::dimensions>{Keys{"dimensions"}, REQUIRED}
        };
    }
};

template<>
struct SchemaTraits<cv::VideoCamera> {
    static auto schema() {
        return Object{
            Field<&cv::VideoCamera::path>{Keys{"path", "file", "video"}, REQUIRED,
                Constraints{NotEmpty, FileExists}}
        };
    }
};

template<>
struct SchemaTraits<cv::UsbCamera> {
    static auto schema() {
        return Object{
            Field<&cv::UsbCamera::device_id>{Keys{"device_id"}, REQUIRED, Constraints{NotEmpty}},
            Field<&cv::UsbCamera::preset_name>{Keys{"preset_name"}, REQUIRED, Constraints{NotEmpty}}
        };
    }
};

template<>
struct SchemaTraits<cv::IpCamera> {
    static auto schema() {
        return Object{
            Field<&cv::IpCamera::ip>{Keys{"ip", "addr", "address"}, REQUIRED,
                Constraints{Or<IsValidIpv4, IsValidHostname>}},
            Field<&cv::IpCamera::endpoint>{Keys{"endpoint"}, Defaults{[] { return std::string{"/"}; }}},
            Field<&cv::IpCamera::password>{Keys{"password"}},
            Field<&cv::IpCamera::username>{Keys{"username"}},
            Field<&cv::IpCamera::port>{Keys{"port"}, REQUIRED, Constraints{Range<1, 65535>}}
        };
    }
};

template<>
struct SchemaTraits<cv::GigeCamera> {
    static auto schema() {
        return Object{
            Field<&cv::GigeCamera::device_id>{Keys{"device_id"}, REQUIRED, Constraints{NotEmpty}},
            Field<&cv::GigeCamera::preset_name>{Keys{"preset_name"}},
            Field<&cv::GigeCamera::mbps>{Keys{"mbps"}, Defaults{Value<cv::GigeCamera::DEFAULT_MBPS>}, Constraints{Positive}},
            Field<&cv::GigeCamera::compression>{Keys{"compression"}, Defaults{Value<false>}},
            Field<&cv::GigeCamera::crop>{Keys{"crop"}}
        };
    }
};

template<>
struct SchemaTraits<cv::Camera> {
    static auto schema() {
        return Variant{Bindings{
            Bind<cv::CameraType::IP, cv::IpCamera>{},
            Bind<cv::CameraType::GIGE, cv::GigeCamera>{},
            Bind<cv::CameraType::USB, cv::UsbCamera>{},
            Bind<cv::CameraType::VIDEO, cv::VideoCamera>{}
        }};
    }
};

template<>
struct SchemaTraits<cv::ClassificationModel> {
    static auto schema() {
        return Object{
            Field<&cv::ClassificationModel::class_names>{Keys{"class_names"}, REQUIRED, Constraints{NotEmpty}}
        };
    }
};

template<>
struct SchemaTraits<cv::DetectionModel> {
    static auto schema() {
        return Object{
            Field<&cv::DetectionModel::class_names>{Keys{"class_names"}, REQUIRED, Constraints{NotEmpty}},
            Field<&cv::DetectionModel::confidence_threshold>{Keys{"confidence_threshold"}, REQUIRED, Constraints{Range<0.0, 1.0>}}
        };
    }
};

template<>
struct SchemaTraits<cv::SegmentationModel> {
    static auto schema() {
        return Object{
            Field<&cv::SegmentationModel::class_names>{Keys{"class_names"}, REQUIRED, Constraints{NotEmpty}},
            Field<&cv::SegmentationModel::mask_confidence_threshold>{Keys{"mask_confidence_threshold"},
                Defaults{Value<0.5>}, Constraints{Range<0.0, 1.0>}}
        };
    }
};

template<>
struct SchemaTraits<cv::Model> {
    static auto schema() {
        return Variant{Bindings{
            Bind<cv::ModelType::CLASSIFICATION, cv::ClassificationModel>{},
            Bind<cv::ModelType::DETECTION, cv::DetectionModel>{},
            Bind<cv::ModelType::SEGMENTATION, cv::SegmentationModel>{}
        }};
    }
};

template<>
struct SchemaTraits<cv::Color> {
    static auto schema() {
        return Object{
            Field<&cv::Color::red>{Keys{"red"}, REQUIRED, Constraints{Range<0, 255>}},
            Field<&cv::Color::green>{Keys{"green"}, REQUIRED, Constraints{Range<0, 255>}},
            Field<&cv::Color::blue>{Keys{"blue"}, REQUIRED, Constraints{Range<0, 255>}}
        };
    }
};

template<>
struct SchemaTraits<cv::Letterbox> {
    static auto schema() {
        return Object{
            Field<&cv::Letterbox::target>{Keys{"target"}, REQUIRED},
            Field<&cv::Letterbox::padding>{Keys{"padding"}, REQUIRED}
        };
    }
};

template<>
struct SchemaTraits<cv::ImageProcessor> {
    static auto schema() {
        return Variant{Bindings{
            Bind<cv::ImageProcessorType::RESIZE, cv::Resize>{},
            Bind<cv::ImageProcessorType::ROI, cv::Roi>{},
            Bind<cv::ImageProcessorType::LETTERBOX, cv::Letterbox>{}
        }};
    }
};

template<>
struct SchemaTraits<cv::CameraNode> {
    static auto schema() {
        return Object{
            Field<&cv::CameraNode::camera>{Keys{"camera"}, REQUIRED}
        };
    }
};

template<>
struct SchemaTraits<cv::ModelNode> {
    static auto schema() {
        return Object{
            Field<&cv::ModelNode::model>{Keys{"model"}, REQUIRED},
            Field<&cv::ModelNode::image_node_name>{Keys{"image_node_name"}, REQUIRED, Constraints{NotEmpty}}
        };
    }
};

template<>
struct SchemaTraits<cv::ImageProcessorNode> {
    static auto schema() {
        return Object{
            Field<&cv::ImageProcessorNode::image_processors>{Keys{"image_processors"}, REQUIRED, Constraints{NotEmpty}},
            Field<&cv::ImageProcessorNode::image_node_name>{Keys{"image_node_name"}, REQUIRED, Constraints{NotEmpty}}
        };
    }
};

template<>
struct SchemaTraits<cv::Node> {
    static auto schema() {
        return Variant{Bindings{
            Bind<cv::NodeType::CAMERA, cv::CameraNode>{},
            Bind<cv::NodeType::MODEL, cv::ModelNode>{},
            Bind<cv::NodeType::PROCESS, cv::ImageProcessorNode>{}
        }};
    }
};

template<>
struct SchemaTraits<cv::PeriodicTrigger> {
    static auto schema() {
        return Object{
            Field<&cv::PeriodicTrigger::scan_every>{Keys{"scan_every_ms"}, REQUIRED, Constraints{test_schemas::PositiveDuration}}
        };
    }
};

template<>
struct SchemaTraits<cv::SerialTrigger> {
    static auto schema() {
        return Object{
            Field<&cv::SerialTrigger::device>{Keys{"device"}, REQUIRED, Constraints{NotEmpty}},
            Field<&cv::SerialTrigger::baud_rate>{Keys{"baud_rate"}, REQUIRED, Constraints{Positive}},
            Field<&cv::SerialTrigger::message>{Keys{"message"}, REQUIRED, Constraints{NotEmpty}}
        };
    }
};

template<>
struct SchemaTraits<cv::HttpTrigger> {
    static auto schema() {
        return Object{
            Field<&cv::HttpTrigger::path>{Keys{"path"}, REQUIRED, Constraints{StartsWith<Str{"/"}>}}
        };
    }
};

template<>
struct SchemaTraits<cv::InspectionTrigger> {
    static auto schema() {
        return Variant{Bindings{
            Bind<cv::TriggerType::PERIODIC, cv::PeriodicTrigger>{},
            Bind<cv::TriggerType::SERIAL, cv::SerialTrigger>{},
            Bind<cv::TriggerType::HTTP, cv::HttpTrigger>{}
        }};
    }
};

template<>
struct SchemaTraits<cv::CommandServer> {
    static auto schema() {
        return Object{
            Field<&cv::CommandServer::ip>{Keys{"ip"}, REQUIRED, Constraints{Or<IsValidIpv4, IsValidHostname>}},
            Field<&cv::CommandServer::port>{Keys{"port"}, REQUIRED, Constraints{Range<1, 65535>}},
            Field<&cv::CommandServer::password>{Keys{"password"}}
        };
    }
};

template<>
struct SchemaTraits<cv::Pipeline> {
    static auto schema() {
        return Object{
            Field<&cv::Pipeline::nodes>{Keys{"nodes"}, REQUIRED, Constraints{NotEmpty, cv::NodeReferencesExist{}}},
            Field<&cv::Pipeline::trigger>{Keys{"trigger"}, REQUIRED}
        };
    }
};

template<>
struct SchemaTraits<cv::Config> {
    static auto schema() {
        return Object{
            Field<&cv::Config::pipelines>{Keys{"pipelines"}, REQUIRED, Constraints{NotEmpty}},
            Field<&cv::Config::command_server>{Keys{"command_server"}}
        };
    }
};

} // namespace struo
