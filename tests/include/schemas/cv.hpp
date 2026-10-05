#pragma once

#include "struo/concepts.hpp"
#include "struo/constraints/platform.hpp"
#include "struo/forward.hpp"
#include "struo/struo.hpp"
#include <chrono>
#include <cstdint>
#include <unordered_map>

namespace cv {
    struct Point {
        uint64_t x{};
        uint64_t y{};
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
        int64_t port{};
        std::optional<std::string> username{};
        std::optional<std::string> password{};
        std::string endpoint{};
    };

    struct GigeCamera {
        static constexpr uint64_t DEFAULT_MBPS { 1000 };

        std::string device_id{};
        std::optional<std::string> preset_name{};
        uint64_t mbps { DEFAULT_MBPS };
        bool compression{};
        std::optional<Region> crop{};
    };

    enum class CameraType {
        VIDEO,
        IP,
        USB,
        GIGE
    };

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
        double mask_confidence_threshold{};
    };

    enum class ModelType {
        CLASSIFICATION,
        DETECTION,
        SEGMENTATION
    };

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

    enum class ImageProcessorType {
        RESIZE,
        ROI,
        LETTERBOX
    };

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

    enum class NodeType {
        CAMERA,
        MODEL,
        PROCESS
    };

    using Node = std::variant<CameraNode, ModelNode, ImageProcessorNode>;

    struct InspectionTrigger {
        std::chrono::milliseconds scan_every{};
    };

    struct CommandServer {
        std::string ip{};
        uint16_t port{};
        std::string password{};
    };
}

namespace struo {

    template<>
    struct SchemaTraits<cv::InspectionTrigger> {
        static constexpr auto schema() {
            return Object{
                Field<&cv::InspectionTrigger::scan_every>{Keys{"scan_every_ms"}}
            };
        }
    };

    template<>
    struct SchemaTraits<cv::Camera> {
        static constexpr auto schema() {
            return Variant{Bindings{
                Bind<cv::CameraType::IP, cv::IpCamera>{},
                Bind<cv::CameraType::GIGE, cv::GigeCamera>{},
                Bind<cv::CameraType::USB, cv::UsbCamera>{},
                Bind<cv::CameraType::VIDEO, cv::VideoCamera>{}
            }};
        }
    };

    template<>
    struct SchemaTraits<cv::VideoCamera> {
        static constexpr auto schema() {
            return Object{
                Field<&cv::VideoCamera::path>{Keys{"path", "file", "video"}, Constraints{FileExists}}
            };
        };
    };

    template<>
    struct SchemaTraits<cv::IpCamera> {
        static constexpr auto schema() {
            return Object {
                Field<&cv::IpCamera::ip>{Keys{"ip", "addr", "address"}, Constraints{Or<IsValidIpv4, IsValidHostname>}},
                Field<&cv::IpCamera::endpoint>{Keys{"endpoint"}, Defaults{Value<Str{"/"}>}},
                Field<&cv::IpCamera::password>{Keys{"password"}},
                Field<&cv::IpCamera::username>{Keys{"username"}},
                Field<&cv::IpCamera::port>{Keys{"port"}, Constraints{IsValidPort}}
            };
        }
    };

    template<>
    struct SchemaTraits<cv::CameraNode> {
        static constexpr auto schema() {
            return Object{
                Field<&cv::CameraNode::camera>{Keys{"camera"}}
            };
        }
    };

}
