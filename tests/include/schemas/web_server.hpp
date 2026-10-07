#pragma once

#include "schemas/constraints.hpp"
#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>
#include <map>
#include <variant>

namespace www {

enum class LogLevel { DEBUG, INFO, WARNING, ERROR };

enum class Strategy { ROUND_ROBIN, RANDOM };

enum class MatchMode { EXACT, PREFIX };

enum class HttpMethod { GET, POST, PUT, PATCH, DELETE, HEAD, OPTIONS };

enum class BeforeType { REQUEST_HEADERS, RATE_LIMIT };

enum class AfterType { RESPONSE_HEADERS };

enum class ActionType { RESPOND, PROXY, REDIRECT, STATIC_FILES };

struct Tls {
    std::filesystem::path certificate{};
    std::filesystem::path private_key{};
};

struct Listener {
    std::string address{"0.0.0.0"};
    int port{};
    std::optional<Tls> tls{};
};

struct Timeouts {
    std::chrono::milliseconds request{30000};
    std::chrono::milliseconds idle{60000};
};

struct Logging {
    LogLevel level{LogLevel::INFO};
    bool access_log{true};
};

struct Endpoint {
    std::string host{};
    int port{};
};

struct Upstream {
    Strategy strategy{Strategy::ROUND_ROBIN};
    std::chrono::milliseconds connect_timeout{2000};
    std::vector<Endpoint> endpoints{};
};

struct RequestHeaders {
    std::map<std::string, std::string> set{};
    std::vector<std::string> remove{};
};

struct ResponseHeaders {
    std::map<std::string, std::string> set{};
    std::vector<std::string> remove{};
};

struct RateLimit {
    int requests{};
    std::chrono::milliseconds window{};
    int rejection_status{429};
};

using BeforeMiddleware = std::variant<RequestHeaders, RateLimit>;

using AfterMiddleware = std::variant<ResponseHeaders>;

struct Middleware {
    std::vector<BeforeMiddleware> before{};
    std::vector<AfterMiddleware> after{};
};

struct Match {
    std::string path{};
    MatchMode mode{MatchMode::EXACT};
    std::optional<std::vector<HttpMethod>> methods{};
};

struct Respond {
    int status{};
    std::string body{};
    std::string content_type{"text/plain"};
};

struct Proxy {
    std::string upstream{};
    bool strip_prefix{false};
};

struct Redirect {
    std::string location{};
    int status{302};
};

struct StaticFiles {
    std::filesystem::path root{};
    std::string index{"index.html"};
    bool directory_listing{false};
};

using Action = std::variant<Respond, Proxy, Redirect, StaticFiles>;

struct Route {
    std::string name{};
    Match match{};
    Middleware middleware{};
    Action action{};
};

struct ErrorPage {
    int status{};
    std::filesystem::path file{};
};

struct Server {
    std::string name{};
    std::vector<Listener> listeners{};
    Timeouts timeouts{};
    Logging logging{};
    std::map<std::string, Upstream> upstreams{};
    Middleware middleware{};
    std::vector<Route> routes{};
    std::vector<ErrorPage> error_pages{};
};

} // namespace www

namespace struo {

template<>
struct SchemaTraits<www::Tls> {
    static auto schema() {
        return Object{
            Field<&www::Tls::certificate>{Keys{"certificate"}, REQUIRED, Constraints{NotEmpty}},
            Field<&www::Tls::private_key>{Keys{"private_key"}, REQUIRED, Constraints{NotEmpty}}
        };
    }
};

template<>
struct SchemaTraits<www::Listener> {
    static auto schema() {
        return Object{
            Field<&www::Listener::address>{Keys{"address"}, Defaults{[] { return std::string{"0.0.0.0"}; }}},
            Field<&www::Listener::port>{Keys{"port"}, REQUIRED, Constraints{Range<1, 65535>}},
            Field<&www::Listener::tls>{Keys{"tls"}}
        };
    }
};

template<>
struct SchemaTraits<www::Timeouts> {
    static auto schema() {
        return Object{
            Field<&www::Timeouts::request>{Keys{"request_ms"}, Defaults{[] { return std::chrono::milliseconds{30000}; }}, Constraints{test_schemas::PositiveDuration}},
            Field<&www::Timeouts::idle>{Keys{"idle_ms"}, Defaults{[] { return std::chrono::milliseconds{60000}; }}, Constraints{test_schemas::PositiveDuration}}
        };
    }
};

template<>
struct SchemaTraits<www::Logging> {
    static auto schema() {
        return Object{
            Field<&www::Logging::level>{Keys{"level"}, Defaults{Value<www::LogLevel::INFO>}},
            Field<&www::Logging::access_log>{Keys{"access_log"}, Defaults{Value<true>}}
        };
    }
};

template<>
struct SchemaTraits<www::Endpoint> {
    static auto schema() {
        return Object{
            Field<&www::Endpoint::host>{Keys{"host"}, REQUIRED, Constraints{NotEmpty}},
            Field<&www::Endpoint::port>{Keys{"port"}, REQUIRED, Constraints{Range<1, 65535>}}
        };
    }
};

template<>
struct SchemaTraits<www::Upstream> {
    static auto schema() {
        return Object{
            Field<&www::Upstream::strategy>{Keys{"strategy"}, Defaults{Value<www::Strategy::ROUND_ROBIN>}},
            Field<&www::Upstream::connect_timeout>{Keys{"connect_timeout_ms"}, Defaults{[] { return std::chrono::milliseconds{2000}; }}, Constraints{test_schemas::PositiveDuration}},
            Field<&www::Upstream::endpoints>{Keys{"endpoints"}, REQUIRED, Constraints{NotEmpty}}
        };
    }
};

template<>
struct SchemaTraits<www::RequestHeaders> {
    static auto schema() {
        return Object{
            Field<&www::RequestHeaders::set>{Keys{"set"}},
            Field<&www::RequestHeaders::remove>{Keys{"remove"}}
        };
    }
};

template<>
struct SchemaTraits<www::ResponseHeaders> {
    static auto schema() {
        return Object{
            Field<&www::ResponseHeaders::set>{Keys{"set"}},
            Field<&www::ResponseHeaders::remove>{Keys{"remove"}}
        };
    }
};

template<>
struct SchemaTraits<www::RateLimit> {
    static auto schema() {
        return Object{
            Field<&www::RateLimit::requests>{Keys{"requests"}, REQUIRED, Constraints{Positive}},
            Field<&www::RateLimit::window>{Keys{"window_ms"}, REQUIRED, Constraints{test_schemas::PositiveDuration}},
            Field<&www::RateLimit::rejection_status>{Keys{"rejection_status"}, Defaults{Value<429>}, Constraints{Range<400, 599>}}
        };
    }
};

template<>
struct SchemaTraits<www::BeforeMiddleware> {
    static auto schema() {
        return Variant{Bindings{
            Bind<www::BeforeType::REQUEST_HEADERS, www::RequestHeaders>{},
            Bind<www::BeforeType::RATE_LIMIT, www::RateLimit>{}
        }};
    }
};

template<>
struct SchemaTraits<www::AfterMiddleware> {
    static auto schema() {
        return Variant{Bindings{
            Bind<www::AfterType::RESPONSE_HEADERS, www::ResponseHeaders>{}
        }};
    }
};

template<>
struct SchemaTraits<www::Middleware> {
    static auto schema() {
        return Object{
            Field<&www::Middleware::before>{Keys{"before"}},
            Field<&www::Middleware::after>{Keys{"after"}}
        };
    }
};

template<>
struct SchemaTraits<www::Match> {
    static auto schema() {
        return Object{
            Field<&www::Match::path>{Keys{"path"}, REQUIRED, Constraints{StartsWith<Str{"/"}>}},
            Field<&www::Match::mode>{Keys{"mode"}, Defaults{Value<www::MatchMode::EXACT>}},
            Field<&www::Match::methods>{Keys{"methods"}, Constraints{[](const auto& methods) { return methods ? NotEmpty(*methods) : ok(); }}}
        };
    }
};

template<>
struct SchemaTraits<www::Respond> {
    static auto schema() {
        return Object{
            Field<&www::Respond::status>{Keys{"status"}, REQUIRED, Constraints{Range<200, 599>}},
            Field<&www::Respond::body>{Keys{"body"}},
            Field<&www::Respond::content_type>{Keys{"content_type"}, Defaults{[] { return std::string{"text/plain"}; }}}
        };
    }
};

template<>
struct SchemaTraits<www::Proxy> {
    static auto schema() {
        return Object{
            Field<&www::Proxy::upstream>{Keys{"upstream"}, REQUIRED, Constraints{NotEmpty}},
            Field<&www::Proxy::strip_prefix>{Keys{"strip_prefix"}, Defaults{Value<false>}}
        };
    }
};

template<>
struct SchemaTraits<www::Redirect> {
    static auto schema() {
        return Object{
            Field<&www::Redirect::location>{Keys{"location"}, REQUIRED, Constraints{NotEmpty}},
            Field<&www::Redirect::status>{Keys{"status"}, Defaults{Value<302>}, Constraints{Or<Range<301, 303>, Range<307, 308>>}}
        };
    }
};

template<>
struct SchemaTraits<www::StaticFiles> {
    static auto schema() {
        return Object{
            Field<&www::StaticFiles::root>{Keys{"root"}, REQUIRED, Constraints{NotEmpty}},
            Field<&www::StaticFiles::index>{Keys{"index"}, Defaults{[] { return std::string{"index.html"}; }}},
            Field<&www::StaticFiles::directory_listing>{Keys{"directory_listing"}, Defaults{Value<false>}}
        };
    }
};

template<>
struct SchemaTraits<www::Action> {
    static auto schema() {
        return Variant{Bindings{
            Bind<www::ActionType::RESPOND, www::Respond>{},
            Bind<www::ActionType::PROXY, www::Proxy>{},
            Bind<www::ActionType::REDIRECT, www::Redirect>{},
            Bind<www::ActionType::STATIC_FILES, www::StaticFiles>{}
        }};
    }
};

template<>
struct SchemaTraits<www::Route> {
    static auto schema() {
        return Object{
            Field<&www::Route::name>{Keys{"name"}, REQUIRED, Constraints{NotEmpty}},
            Field<&www::Route::match>{Keys{"match"}, REQUIRED},
            Field<&www::Route::middleware>{Keys{"middleware"}},
            Field<&www::Route::action>{Keys{"action"}, REQUIRED}
        };
    }
};

template<>
struct SchemaTraits<www::ErrorPage> {
    static auto schema() {
        return Object{
            Field<&www::ErrorPage::status>{Keys{"status"}, REQUIRED, Constraints{Range<400, 599>}},
            Field<&www::ErrorPage::file>{Keys{"file"}, REQUIRED, Constraints{NotEmpty}}
        };
    }
};

template<>
struct SchemaTraits<www::Server> {
    static auto schema() {
        return Object{
            Field<&www::Server::name>{Keys{"name"}},
            Field<&www::Server::listeners>{Keys{"listeners"}, REQUIRED, Constraints{NotEmpty}},
            Field<&www::Server::timeouts>{Keys{"timeouts"}},
            Field<&www::Server::logging>{Keys{"logging"}},
            Field<&www::Server::upstreams>{Keys{"upstreams"}},
            Field<&www::Server::middleware>{Keys{"middleware"}},
            Field<&www::Server::routes>{Keys{"routes"}, REQUIRED, Constraints{NotEmpty}},
            Field<&www::Server::error_pages>{Keys{"error_pages"}}
        };
    }
};

} // namespace struo
