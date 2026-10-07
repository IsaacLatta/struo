#include <chrono>
#include <filesystem>
#include <map>
#include <string>
#include <variant>
#include <vector>

#include "e2e/fixtures.hpp"
#include "schemas/web_server.hpp"

namespace {

using namespace std::chrono_literals;

template<typename Descriptor>
class WebServer : public e2e::FixtureTest<www::Server, Descriptor> {};

TYPED_TEST_SUITE(WebServer, e2e::AllFormats);

void expect_empty_middleware(const www::Middleware& middleware) {
    EXPECT_TRUE(middleware.before.empty());
    EXPECT_TRUE(middleware.after.empty());
}

void expect_default_sections(const www::Server& server) {
    EXPECT_EQ(server.timeouts.request, 30000ms);
    EXPECT_EQ(server.timeouts.idle, 60000ms);
    EXPECT_EQ(server.logging.level, www::LogLevel::INFO);
    EXPECT_TRUE(server.logging.access_log);
    ASSERT_EQ(server.listeners.size(), 1u);
    EXPECT_EQ(server.listeners[0].address, "0.0.0.0");
    EXPECT_EQ(server.listeners[0].port, 8080);
    EXPECT_FALSE(server.listeners[0].tls);
    expect_empty_middleware(server.middleware);
    EXPECT_TRUE(server.error_pages.empty());
}

TYPED_TEST(WebServer, FullConfiguration) {
    const auto result = this->loadCase("web_server", "full");
    ASSERT_TRUE(result) << result.error().what();
    const auto& server = *result;
    EXPECT_EQ(server.name, "edge");
    ASSERT_EQ(server.listeners.size(), 2u);
    EXPECT_EQ(server.listeners[0].address, "127.0.0.1");
    EXPECT_EQ(server.listeners[0].port, 8080);
    EXPECT_FALSE(server.listeners[0].tls);
    EXPECT_EQ(server.listeners[1].address, "0.0.0.0");
    EXPECT_EQ(server.listeners[1].port, 8443);
    ASSERT_TRUE(server.listeners[1].tls);
    EXPECT_EQ(server.listeners[1].tls->certificate, std::filesystem::path{"certs/server.pem"});
    EXPECT_EQ(server.listeners[1].tls->private_key, std::filesystem::path{"certs/server.key"});
    EXPECT_EQ(server.timeouts.request, 12000ms);
    EXPECT_EQ(server.timeouts.idle, 45000ms);
    EXPECT_EQ(server.logging.level, www::LogLevel::DEBUG);
    EXPECT_FALSE(server.logging.access_log);

    ASSERT_EQ(server.upstreams.size(), 1u);
    ASSERT_TRUE(server.upstreams.contains("api"));
    const auto& upstream = server.upstreams.at("api");
    EXPECT_EQ(upstream.strategy, www::Strategy::RANDOM);
    EXPECT_EQ(upstream.connect_timeout, 1500ms);
    ASSERT_EQ(upstream.endpoints.size(), 2u);
    EXPECT_EQ(upstream.endpoints[0].host, "api-a.local");
    EXPECT_EQ(upstream.endpoints[0].port, 9000);
    EXPECT_EQ(upstream.endpoints[1].host, "api-b.local");
    EXPECT_EQ(upstream.endpoints[1].port, 9001);

    ASSERT_EQ(server.middleware.before.size(), 2u);
    ASSERT_TRUE(std::holds_alternative<www::RequestHeaders>(server.middleware.before[0]));
    const auto& headers = std::get<www::RequestHeaders>(server.middleware.before[0]);
    EXPECT_EQ(headers.set, (std::map<std::string, std::string>{{"X-Request-Source", "edge"}}));
    EXPECT_EQ(headers.remove, (std::vector<std::string>{"X-Internal"}));
    ASSERT_TRUE(std::holds_alternative<www::RateLimit>(server.middleware.before[1]));
    const auto& limit = std::get<www::RateLimit>(server.middleware.before[1]);
    EXPECT_EQ(limit.requests, 100);
    EXPECT_EQ(limit.window, 60000ms);
    EXPECT_EQ(limit.rejection_status, 503);
    ASSERT_EQ(server.middleware.after.size(), 1u);
    ASSERT_TRUE(std::holds_alternative<www::ResponseHeaders>(server.middleware.after[0]));
    const auto& response_headers = std::get<www::ResponseHeaders>(server.middleware.after[0]);
    EXPECT_EQ(response_headers.set, (std::map<std::string, std::string>{{"X-Served-By", "struo"}}));
    EXPECT_EQ(response_headers.remove, (std::vector<std::string>{"X-Powered-By"}));

    ASSERT_EQ(server.routes.size(), 4u);
    const auto& health = server.routes[0];
    EXPECT_EQ(health.name, "health");
    EXPECT_EQ(health.match.path, "/health");
    EXPECT_EQ(health.match.mode, www::MatchMode::EXACT);
    ASSERT_TRUE(health.match.methods);
    EXPECT_EQ(*health.match.methods, (std::vector<www::HttpMethod>{www::HttpMethod::GET, www::HttpMethod::HEAD}));
    ASSERT_TRUE(std::holds_alternative<www::Respond>(health.action));
    const auto& respond = std::get<www::Respond>(health.action);
    EXPECT_EQ(respond.status, 200);
    EXPECT_EQ(respond.body, R"({"status":"ok"})");
    EXPECT_EQ(respond.content_type, "application/json");
    ASSERT_EQ(health.middleware.before.size(), 1u);
    ASSERT_TRUE(std::holds_alternative<www::RateLimit>(health.middleware.before[0]));
    const auto& health_limit = std::get<www::RateLimit>(health.middleware.before[0]);
    EXPECT_EQ(health_limit.requests, 30);
    EXPECT_EQ(health_limit.window, 1000ms);
    EXPECT_EQ(health_limit.rejection_status, 429);
    ASSERT_EQ(health.middleware.after.size(), 1u);
    ASSERT_TRUE(std::holds_alternative<www::ResponseHeaders>(health.middleware.after[0]));
    const auto& health_headers = std::get<www::ResponseHeaders>(health.middleware.after[0]);
    EXPECT_EQ(health_headers.set, (std::map<std::string, std::string>{{"X-Health", "true"}}));
    EXPECT_TRUE(health_headers.remove.empty());

    const auto& api = server.routes[1];
    EXPECT_EQ(api.name, "api");
    EXPECT_EQ(api.match.path, "/api");
    EXPECT_EQ(api.match.mode, www::MatchMode::PREFIX);
    ASSERT_TRUE(api.match.methods);
    EXPECT_EQ(*api.match.methods, (std::vector<www::HttpMethod>{
        www::HttpMethod::GET, www::HttpMethod::POST, www::HttpMethod::PUT,
        www::HttpMethod::PATCH, www::HttpMethod::DELETE, www::HttpMethod::OPTIONS}));
    ASSERT_TRUE(std::holds_alternative<www::Proxy>(api.action));
    const auto& proxy = std::get<www::Proxy>(api.action);
    EXPECT_EQ(proxy.upstream, "api");
    EXPECT_TRUE(proxy.strip_prefix);
    ASSERT_EQ(api.middleware.before.size(), 1u);
    ASSERT_TRUE(std::holds_alternative<www::RequestHeaders>(api.middleware.before[0]));
    const auto& proxy_headers = std::get<www::RequestHeaders>(api.middleware.before[0]);
    EXPECT_EQ(proxy_headers.set, (std::map<std::string, std::string>{{"X-Proxy-Mode", "api"}}));
    EXPECT_EQ(proxy_headers.remove, (std::vector<std::string>{"X-Old-Proxy"}));
    EXPECT_TRUE(api.middleware.after.empty());

    const auto& redirect_route = server.routes[2];
    EXPECT_EQ(redirect_route.name, "redirect");
    EXPECT_EQ(redirect_route.match.path, "/old");
    EXPECT_EQ(redirect_route.match.mode, www::MatchMode::EXACT);
    ASSERT_TRUE(redirect_route.match.methods);
    EXPECT_EQ(*redirect_route.match.methods, (std::vector<www::HttpMethod>{www::HttpMethod::GET}));
    expect_empty_middleware(redirect_route.middleware);
    ASSERT_TRUE(std::holds_alternative<www::Redirect>(redirect_route.action));
    const auto& redirect = std::get<www::Redirect>(redirect_route.action);
    EXPECT_EQ(redirect.location, "/new");
    EXPECT_EQ(redirect.status, 308);

    const auto& assets = server.routes[3];
    EXPECT_EQ(assets.name, "assets");
    EXPECT_EQ(assets.match.path, "/assets");
    EXPECT_EQ(assets.match.mode, www::MatchMode::PREFIX);
    ASSERT_TRUE(assets.match.methods);
    EXPECT_EQ(*assets.match.methods, (std::vector<www::HttpMethod>{www::HttpMethod::GET, www::HttpMethod::HEAD}));
    expect_empty_middleware(assets.middleware);
    ASSERT_TRUE(std::holds_alternative<www::StaticFiles>(assets.action));
    const auto& files = std::get<www::StaticFiles>(assets.action);
    EXPECT_EQ(files.root, std::filesystem::path{"public"});
    EXPECT_EQ(files.index, "home.html");
    EXPECT_TRUE(files.directory_listing);

    ASSERT_EQ(server.error_pages.size(), 2u);
    EXPECT_EQ(server.error_pages[0].status, 404);
    EXPECT_EQ(server.error_pages[0].file, std::filesystem::path{"errors/404.html"});
    EXPECT_EQ(server.error_pages[1].status, 503);
    EXPECT_EQ(server.error_pages[1].file, std::filesystem::path{"errors/503.html"});
}

TYPED_TEST(WebServer, MinimalConfiguration) {
    const auto result = this->loadCase("web_server", "minimal");
    ASSERT_TRUE(result) << result.error().what();
    const auto& server = *result;
    EXPECT_TRUE(server.name.empty());
    expect_default_sections(server);
    EXPECT_TRUE(server.upstreams.empty());
    ASSERT_EQ(server.routes.size(), 1u);
    const auto& route = server.routes[0];
    EXPECT_EQ(route.name, "health");
    EXPECT_EQ(route.match.path, "/health");
    EXPECT_EQ(route.match.mode, www::MatchMode::EXACT);
    EXPECT_FALSE(route.match.methods);
    expect_empty_middleware(route.middleware);
    ASSERT_TRUE(std::holds_alternative<www::Respond>(route.action));
    const auto& respond = std::get<www::Respond>(route.action);
    EXPECT_EQ(respond.status, 200);
    EXPECT_TRUE(respond.body.empty());
    EXPECT_EQ(respond.content_type, "text/plain");
}

TYPED_TEST(WebServer, NestedDefaults) {
    const auto result = this->loadCase("web_server", "nested_defaults");
    ASSERT_TRUE(result) << result.error().what();
    const auto& server = *result;
    EXPECT_EQ(server.name, "defaults");
    expect_default_sections(server);
    ASSERT_EQ(server.upstreams.size(), 1u);
    ASSERT_TRUE(server.upstreams.contains("api"));
    const auto& upstream = server.upstreams.at("api");
    EXPECT_EQ(upstream.strategy, www::Strategy::ROUND_ROBIN);
    EXPECT_EQ(upstream.connect_timeout, 2000ms);
    ASSERT_EQ(upstream.endpoints.size(), 1u);
    EXPECT_EQ(upstream.endpoints[0].host, "service.local");
    EXPECT_EQ(upstream.endpoints[0].port, 9000);
    ASSERT_EQ(server.routes.size(), 4u);
    const std::vector<std::string> names{"health", "api", "redirect", "assets"};
    const std::vector<std::string> paths{"/health", "/api", "/old", "/assets"};
    for (std::size_t i = 0; i < server.routes.size(); ++i) {
        SCOPED_TRACE(i);
        EXPECT_EQ(server.routes[i].name, names[i]);
        EXPECT_EQ(server.routes[i].match.path, paths[i]);
        EXPECT_EQ(server.routes[i].match.mode, www::MatchMode::EXACT);
        EXPECT_FALSE(server.routes[i].match.methods);
        expect_empty_middleware(server.routes[i].middleware);
    }
    ASSERT_TRUE(std::holds_alternative<www::Respond>(server.routes[0].action));
    const auto& respond = std::get<www::Respond>(server.routes[0].action);
    EXPECT_EQ(respond.status, 200);
    EXPECT_TRUE(respond.body.empty());
    EXPECT_EQ(respond.content_type, "text/plain");
    ASSERT_TRUE(std::holds_alternative<www::Proxy>(server.routes[1].action));
    const auto& proxy = std::get<www::Proxy>(server.routes[1].action);
    EXPECT_EQ(proxy.upstream, "api");
    EXPECT_FALSE(proxy.strip_prefix);
    ASSERT_TRUE(std::holds_alternative<www::Redirect>(server.routes[2].action));
    const auto& redirect = std::get<www::Redirect>(server.routes[2].action);
    EXPECT_EQ(redirect.location, "/new");
    EXPECT_EQ(redirect.status, 302);
    ASSERT_TRUE(std::holds_alternative<www::StaticFiles>(server.routes[3].action));
    const auto& files = std::get<www::StaticFiles>(server.routes[3].action);
    EXPECT_EQ(files.root, std::filesystem::path{"public"});
    EXPECT_EQ(files.index, "index.html");
    EXPECT_FALSE(files.directory_listing);
}

TYPED_TEST(WebServer, MissingRequiredField) {
    EXPECT_FALSE(this->loadCase("web_server", "missing_required"));
}

TYPED_TEST(WebServer, WrongStructuralType) {
    EXPECT_FALSE(this->loadCase("web_server", "wrong_type"));
}

TYPED_TEST(WebServer, InvalidConstraint) {
    EXPECT_FALSE(this->loadCase("web_server", "invalid_constraint"));
}

TYPED_TEST(WebServer, UnknownVariant) {
    EXPECT_FALSE(this->loadCase("web_server", "unknown_variant"));
}

TYPED_TEST(WebServer, MissingVariantPayload) {
    EXPECT_FALSE(this->loadCase("web_server", "missing_variant_payload"));
}

TYPED_TEST(WebServer, MalformedDocument) {
    EXPECT_FALSE(this->loadCase("web_server", "malformed", false));
}

} // namespace
