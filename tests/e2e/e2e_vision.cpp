#include <chrono>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <variant>

#include "e2e/fixtures.hpp"
#include "schemas/cv.hpp"

namespace {

using namespace std::chrono_literals;
using Classes = std::unordered_map<std::string, int>;

template<typename Descriptor>
class Vision : public e2e::FixtureTest<cv::Config, Descriptor> {};

TYPED_TEST_SUITE(Vision, e2e::AllFormats);

void expect_periodic(const cv::Pipeline& pipeline, std::chrono::milliseconds interval) {
    ASSERT_TRUE(std::holds_alternative<cv::PeriodicTrigger>(pipeline.trigger));
    EXPECT_EQ(std::get<cv::PeriodicTrigger>(pipeline.trigger).scan_every, interval);
}

void expect_classification(const cv::ModelNode& node, const Classes& classes) {
    EXPECT_EQ(node.image_node_name, "source");
    ASSERT_TRUE(std::holds_alternative<cv::ClassificationModel>(node.model));
    EXPECT_EQ(std::get<cv::ClassificationModel>(node.model).class_names, classes);
}

void expect_ip_defaults(const cv::IpCamera& camera, const std::string& address, int port) {
    EXPECT_EQ(camera.ip, address);
    EXPECT_EQ(camera.port, port);
    EXPECT_EQ(camera.endpoint, "/");
    EXPECT_FALSE(camera.username);
    EXPECT_FALSE(camera.password);
}

TYPED_TEST(Vision, FullConfiguration) {
    const auto result = this->loadCase("vision", "full");
    ASSERT_TRUE(result) << result.error().what();
    const auto& config = *result;
    ASSERT_TRUE(config.command_server);
    EXPECT_EQ(config.command_server->ip, "127.0.0.1");
    EXPECT_EQ(config.command_server->port, 9090);
    ASSERT_TRUE(config.command_server->password);
    EXPECT_EQ(*config.command_server->password, "fixture-admin");
    ASSERT_EQ(config.pipelines.size(), 3u);
    ASSERT_TRUE(config.pipelines.contains("inspection"));
    ASSERT_TRUE(config.pipelines.contains("sorting"));
    ASSERT_TRUE(config.pipelines.contains("remote"));

    const auto& inspection = config.pipelines.at("inspection");
    expect_periodic(inspection, 250ms);
    ASSERT_EQ(inspection.nodes.size(), 3u);
    ASSERT_TRUE(inspection.nodes.contains("source"));
    ASSERT_TRUE(inspection.nodes.contains("prepare"));
    ASSERT_TRUE(inspection.nodes.contains("model"));
    ASSERT_TRUE(std::holds_alternative<cv::CameraNode>(inspection.nodes.at("source")));
    const auto& camera = std::get<cv::CameraNode>(inspection.nodes.at("source")).camera;
    ASSERT_TRUE(std::holds_alternative<cv::GigeCamera>(camera));
    const auto& gige = std::get<cv::GigeCamera>(camera);
    EXPECT_EQ(gige.device_id, "gige-01");
    ASSERT_TRUE(gige.preset_name);
    EXPECT_EQ(*gige.preset_name, "inspection");
    EXPECT_EQ(gige.mbps, 2500u);
    EXPECT_TRUE(gige.compression);
    ASSERT_TRUE(gige.crop);
    EXPECT_EQ(gige.crop->origin.x, 8u);
    EXPECT_EQ(gige.crop->origin.y, 12u);
    EXPECT_EQ(gige.crop->dimensions.height, 720);
    EXPECT_EQ(gige.crop->dimensions.width, 1280);

    ASSERT_TRUE(std::holds_alternative<cv::ImageProcessorNode>(inspection.nodes.at("prepare")));
    const auto& prepare = std::get<cv::ImageProcessorNode>(inspection.nodes.at("prepare"));
    EXPECT_EQ(prepare.image_node_name, "source");
    ASSERT_EQ(prepare.image_processors.size(), 3u);
    ASSERT_TRUE(std::holds_alternative<cv::Roi>(prepare.image_processors[0]));
    const auto& roi = std::get<cv::Roi>(prepare.image_processors[0]);
    EXPECT_EQ(roi.origin.x, 10u);
    EXPECT_EQ(roi.origin.y, 20u);
    EXPECT_EQ(roi.dimensions.height, 480);
    EXPECT_EQ(roi.dimensions.width, 640);
    ASSERT_TRUE(std::holds_alternative<cv::Resize>(prepare.image_processors[1]));
    const auto& resize = std::get<cv::Resize>(prepare.image_processors[1]);
    EXPECT_EQ(resize.height, 224);
    EXPECT_EQ(resize.width, 224);
    ASSERT_TRUE(std::holds_alternative<cv::Letterbox>(prepare.image_processors[2]));
    const auto& letterbox = std::get<cv::Letterbox>(prepare.image_processors[2]);
    EXPECT_EQ(letterbox.target.height, 256);
    EXPECT_EQ(letterbox.target.width, 256);
    EXPECT_EQ(letterbox.padding.red, 114);
    EXPECT_EQ(letterbox.padding.green, 115);
    EXPECT_EQ(letterbox.padding.blue, 116);

    ASSERT_TRUE(std::holds_alternative<cv::ModelNode>(inspection.nodes.at("model")));
    const auto& detection_node = std::get<cv::ModelNode>(inspection.nodes.at("model"));
    EXPECT_EQ(detection_node.image_node_name, "prepare");
    ASSERT_TRUE(std::holds_alternative<cv::DetectionModel>(detection_node.model));
    const auto& detection = std::get<cv::DetectionModel>(detection_node.model);
    EXPECT_EQ(detection.class_names, (Classes{{"defect", 0}, {"ok", 1}}));
    EXPECT_DOUBLE_EQ(detection.confidence_threshold, 0.8);

    const auto& sorting = config.pipelines.at("sorting");
    ASSERT_EQ(sorting.nodes.size(), 2u);
    ASSERT_TRUE(sorting.nodes.contains("source"));
    ASSERT_TRUE(sorting.nodes.contains("model"));
    ASSERT_TRUE(std::holds_alternative<cv::CameraNode>(sorting.nodes.at("source")));
    const auto& usb_camera = std::get<cv::CameraNode>(sorting.nodes.at("source")).camera;
    ASSERT_TRUE(std::holds_alternative<cv::UsbCamera>(usb_camera));
    const auto& usb = std::get<cv::UsbCamera>(usb_camera);
    EXPECT_EQ(usb.device_id, "usb-01");
    EXPECT_EQ(usb.preset_name, "production");
    ASSERT_TRUE(std::holds_alternative<cv::ModelNode>(sorting.nodes.at("model")));
    expect_classification(std::get<cv::ModelNode>(sorting.nodes.at("model")), Classes{{"accept", 0}, {"reject", 1}});
    ASSERT_TRUE(std::holds_alternative<cv::SerialTrigger>(sorting.trigger));
    const auto& serial = std::get<cv::SerialTrigger>(sorting.trigger);
    EXPECT_EQ(serial.device, std::filesystem::path{"/dev/ttyTEST"});
    EXPECT_EQ(serial.baud_rate, 115200u);
    EXPECT_EQ(serial.message, "SCAN");

    const auto& remote = config.pipelines.at("remote");
    ASSERT_EQ(remote.nodes.size(), 2u);
    ASSERT_TRUE(remote.nodes.contains("source"));
    ASSERT_TRUE(remote.nodes.contains("model"));
    ASSERT_TRUE(std::holds_alternative<cv::CameraNode>(remote.nodes.at("source")));
    const auto& ip_camera = std::get<cv::CameraNode>(remote.nodes.at("source")).camera;
    ASSERT_TRUE(std::holds_alternative<cv::IpCamera>(ip_camera));
    const auto& ip = std::get<cv::IpCamera>(ip_camera);
    EXPECT_EQ(ip.ip, "192.0.2.10");
    EXPECT_EQ(ip.port, 554);
    EXPECT_EQ(ip.endpoint, "/stream");
    ASSERT_TRUE(ip.username);
    EXPECT_EQ(*ip.username, "fixture-user");
    ASSERT_TRUE(ip.password);
    EXPECT_EQ(*ip.password, "fixture-password");
    ASSERT_TRUE(std::holds_alternative<cv::ModelNode>(remote.nodes.at("model")));
    const auto& segmentation_node = std::get<cv::ModelNode>(remote.nodes.at("model"));
    EXPECT_EQ(segmentation_node.image_node_name, "source");
    ASSERT_TRUE(std::holds_alternative<cv::SegmentationModel>(segmentation_node.model));
    const auto& segmentation = std::get<cv::SegmentationModel>(segmentation_node.model);
    EXPECT_EQ(segmentation.class_names, (Classes{{"background", 0}, {"part", 1}}));
    EXPECT_DOUBLE_EQ(segmentation.mask_confidence_threshold, 0.7);
    ASSERT_TRUE(std::holds_alternative<cv::HttpTrigger>(remote.trigger));
    EXPECT_EQ(std::get<cv::HttpTrigger>(remote.trigger).path, "/inspect");
}

TYPED_TEST(Vision, MinimalConfiguration) {
    const auto result = this->loadCase("vision", "minimal");
    ASSERT_TRUE(result) << result.error().what();
    const auto& config = *result;
    EXPECT_FALSE(config.command_server);
    ASSERT_EQ(config.pipelines.size(), 1u);
    ASSERT_TRUE(config.pipelines.contains("inspection"));
    const auto& pipeline = config.pipelines.at("inspection");
    expect_periodic(pipeline, 1000ms);
    ASSERT_EQ(pipeline.nodes.size(), 2u);
    ASSERT_TRUE(pipeline.nodes.contains("source"));
    ASSERT_TRUE(pipeline.nodes.contains("model"));
    ASSERT_TRUE(std::holds_alternative<cv::CameraNode>(pipeline.nodes.at("source")));
    const auto& camera = std::get<cv::CameraNode>(pipeline.nodes.at("source")).camera;
    ASSERT_TRUE(std::holds_alternative<cv::IpCamera>(camera));
    expect_ip_defaults(std::get<cv::IpCamera>(camera), "192.0.2.20", 554);
    ASSERT_TRUE(std::holds_alternative<cv::ModelNode>(pipeline.nodes.at("model")));
    expect_classification(std::get<cv::ModelNode>(pipeline.nodes.at("model")), Classes{{"ok", 0}, {"reject", 1}});
}

TYPED_TEST(Vision, DefaultsAndLocalNodeNames) {
    const auto result = this->loadCase("vision", "defaults");
    ASSERT_TRUE(result) << result.error().what();
    const auto& config = *result;
    ASSERT_TRUE(config.command_server);
    EXPECT_EQ(config.command_server->ip, "127.0.0.1");
    EXPECT_EQ(config.command_server->port, 9091);
    EXPECT_FALSE(config.command_server->password);
    ASSERT_EQ(config.pipelines.size(), 2u);
    ASSERT_TRUE(config.pipelines.contains("ip_defaults"));
    ASSERT_TRUE(config.pipelines.contains("gige_defaults"));
    const auto& ip_pipeline = config.pipelines.at("ip_defaults");
    expect_periodic(ip_pipeline, 500ms);
    ASSERT_EQ(ip_pipeline.nodes.size(), 2u);
    ASSERT_TRUE(ip_pipeline.nodes.contains("source"));
    ASSERT_TRUE(ip_pipeline.nodes.contains("model"));
    ASSERT_TRUE(std::holds_alternative<cv::CameraNode>(ip_pipeline.nodes.at("source")));
    const auto& ip_camera = std::get<cv::CameraNode>(ip_pipeline.nodes.at("source")).camera;
    ASSERT_TRUE(std::holds_alternative<cv::IpCamera>(ip_camera));
    expect_ip_defaults(std::get<cv::IpCamera>(ip_camera), "camera.local", 8554);
    ASSERT_TRUE(std::holds_alternative<cv::ModelNode>(ip_pipeline.nodes.at("model")));
    expect_classification(std::get<cv::ModelNode>(ip_pipeline.nodes.at("model")), Classes{{"ok", 0}});

    const auto& gige_pipeline = config.pipelines.at("gige_defaults");
    expect_periodic(gige_pipeline, 750ms);
    ASSERT_EQ(gige_pipeline.nodes.size(), 2u);
    ASSERT_TRUE(gige_pipeline.nodes.contains("source"));
    ASSERT_TRUE(gige_pipeline.nodes.contains("model"));
    ASSERT_TRUE(std::holds_alternative<cv::CameraNode>(gige_pipeline.nodes.at("source")));
    const auto& gige_camera = std::get<cv::CameraNode>(gige_pipeline.nodes.at("source")).camera;
    ASSERT_TRUE(std::holds_alternative<cv::GigeCamera>(gige_camera));
    const auto& gige = std::get<cv::GigeCamera>(gige_camera);
    EXPECT_EQ(gige.device_id, "gige-default");
    EXPECT_EQ(gige.mbps, 1000u);
    EXPECT_FALSE(gige.compression);
    EXPECT_FALSE(gige.preset_name);
    EXPECT_FALSE(gige.crop);
    ASSERT_TRUE(std::holds_alternative<cv::ModelNode>(gige_pipeline.nodes.at("model")));
    const auto& model_node = std::get<cv::ModelNode>(gige_pipeline.nodes.at("model"));
    EXPECT_EQ(model_node.image_node_name, "source");
    ASSERT_TRUE(std::holds_alternative<cv::SegmentationModel>(model_node.model));
    const auto& segmentation = std::get<cv::SegmentationModel>(model_node.model);
    EXPECT_EQ(segmentation.class_names, (Classes{{"background", 0}, {"part", 1}}));
    EXPECT_DOUBLE_EQ(segmentation.mask_confidence_threshold, 0.5);
}

TYPED_TEST(Vision, VideoPathAlias) {
    const auto result = this->loadCase("vision", "video");
    ASSERT_TRUE(result) << result.error().what();
    const auto& config = *result;
    EXPECT_FALSE(config.command_server);
    ASSERT_EQ(config.pipelines.size(), 1u);
    ASSERT_TRUE(config.pipelines.contains("replay"));
    const auto& pipeline = config.pipelines.at("replay");
    expect_periodic(pipeline, 40ms);
    ASSERT_EQ(pipeline.nodes.size(), 2u);
    ASSERT_TRUE(pipeline.nodes.contains("source"));
    ASSERT_TRUE(pipeline.nodes.contains("model"));
    ASSERT_TRUE(std::holds_alternative<cv::CameraNode>(pipeline.nodes.at("source")));
    const auto& camera = std::get<cv::CameraNode>(pipeline.nodes.at("source")).camera;
    ASSERT_TRUE(std::holds_alternative<cv::VideoCamera>(camera));
    EXPECT_EQ(std::get<cv::VideoCamera>(camera).path, std::filesystem::path{"tests/fixtures/resources/video.txt"});
    ASSERT_TRUE(std::holds_alternative<cv::ModelNode>(pipeline.nodes.at("model")));
    expect_classification(std::get<cv::ModelNode>(pipeline.nodes.at("model")), Classes{{"frame", 0}});
}

TYPED_TEST(Vision, MissingRequiredField) {
    EXPECT_FALSE(this->loadCase("vision", "missing_required"));
}

TYPED_TEST(Vision, WrongStructuralType) {
    EXPECT_FALSE(this->loadCase("vision", "wrong_type"));
}

TYPED_TEST(Vision, InvalidConstraint) {
    EXPECT_FALSE(this->loadCase("vision", "invalid_constraint"));
}

TYPED_TEST(Vision, UnknownVariant) {
    EXPECT_FALSE(this->loadCase("vision", "unknown_variant"));
}

TYPED_TEST(Vision, MissingVariantPayload) {
    EXPECT_FALSE(this->loadCase("vision", "missing_variant_payload"));
}

TYPED_TEST(Vision, MissingLocalReference) {
    EXPECT_FALSE(this->loadCase("vision", "missing_reference"));
}

TYPED_TEST(Vision, RejectsCrossPipelineReference) {
    EXPECT_FALSE(this->loadCase("vision", "cross_pipeline_reference"));
}

TYPED_TEST(Vision, MalformedDocument) {
    EXPECT_FALSE(this->loadCase("vision", "malformed", false));
}

} // namespace
