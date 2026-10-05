#pragma once

#include "schemas/constraints.hpp"
#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>
#include <map>
#include <variant>

namespace game {

using Milliseconds = std::chrono::milliseconds;

enum class DisplayMode { WINDOWED, FULLSCREEN, BORDERLESS };

enum class EdgeMode { WRAP, SOLID };

enum class Direction { UP, DOWN, LEFT, RIGHT };

enum class EffectType { SPEED, SHIELD, SCORE_MULTIPLIER };

enum class BehaviorType { WANDER, CHASE, PATROL };

struct Position {
    int x{};
    int y{};
};

struct Video {
    int cell_size{24};
    DisplayMode mode{DisplayMode::WINDOWED};
    bool vsync{true};
};

struct Audio {
    double master_volume{1.0};
    double music_volume{0.5};
    double effects_volume{1.0};
    bool muted{false};
};

struct Board {
    int width{30};
    int height{20};
    EdgeMode edges{EdgeMode::SOLID};
    std::vector<Position> obstacles{};
};

struct Snake {
    int starting_length{3};
    Position starting_position{5, 10};
    Direction starting_direction{Direction::RIGHT};
    Milliseconds move_interval{150};
};

struct Fruit {
    int points{10};
    int growth{1};
    int spawn_weight{1};
    std::optional<Milliseconds> expires_after{};
};

struct Speed {
    double multiplier{};
    Milliseconds duration{};
};

struct Shield {
    int charges{1};
    Milliseconds duration{};
};

struct ScoreMultiplier {
    int multiplier{};
    Milliseconds duration{};
};

using Effect = std::variant<Speed, Shield, ScoreMultiplier>;

struct Powerup {
    int spawn_weight{1};
    std::optional<Milliseconds> expires_after{};
    Effect effect{};
};

struct Wander {
    double turn_chance{0.25};
};

struct Chase {
    int detection_radius{6};
};

struct Patrol {
    std::vector<Position> waypoints{};
    bool loop{true};
};

using Behavior = std::variant<Wander, Chase, Patrol>;

struct Enemy {
    Milliseconds move_interval{500};
    Behavior behavior{};
};

struct FruitSpawn {
    int max_active{1};
    Milliseconds interval{1000};
};

struct PowerupSpawn {
    int max_active{0};
    Milliseconds interval{10000};
};

struct EnemySpawn {
    int max_active{0};
    Milliseconds interval{15000};
};

struct Spawning {
    FruitSpawn fruit{};
    PowerupSpawn powerups{};
    EnemySpawn enemies{};
};

struct Game {
    std::string name{"Snake"};
    Video video{};
    Audio audio{};
    Board board{};
    Snake snake{};
    std::map<std::string, Fruit> fruits{};
    std::map<std::string, Powerup> powerups{};
    std::map<std::string, Enemy> enemies{};
    Spawning spawning{};
};

} // namespace game

namespace struo {

template<>
struct SchemaTraits<game::Position> {
    static auto schema() {
        return Object{
            Field<&game::Position::x>{Keys{"x"}, Constraints{AtLeast<0>}},
            Field<&game::Position::y>{Keys{"y"}, Constraints{AtLeast<0>}}
        };
    }
};

template<>
struct SchemaTraits<game::Video> {
    static auto schema() {
        return Object{
            Field<&game::Video::cell_size>{Keys{"cell_size"}, Defaults{Value<24>}, Constraints{AtLeast<1>}},
            Field<&game::Video::mode>{Keys{"mode"}, Defaults{Value<game::DisplayMode::WINDOWED>}},
            Field<&game::Video::vsync>{Keys{"vsync"}, Defaults{Value<true>}}
        };
    }
};

template<>
struct SchemaTraits<game::Audio> {
    static auto schema() {
        return Object{
            Field<&game::Audio::master_volume>{Keys{"master_volume"}, Defaults{Value<1.0>}, Constraints{Range<0.0, 1.0>}},
            Field<&game::Audio::music_volume>{Keys{"music_volume"}, Defaults{Value<0.5>}, Constraints{Range<0.0, 1.0>}},
            Field<&game::Audio::effects_volume>{Keys{"effects_volume"}, Defaults{Value<1.0>}, Constraints{Range<0.0, 1.0>}},
            Field<&game::Audio::muted>{Keys{"muted"}, Defaults{Value<false>}}
        };
    }
};

template<>
struct SchemaTraits<game::Board> {
    static auto schema() {
        return Object{
            Field<&game::Board::width>{Keys{"width"}, Defaults{Value<30>}, Constraints{AtLeast<1>}},
            Field<&game::Board::height>{Keys{"height"}, Defaults{Value<20>}, Constraints{AtLeast<1>}},
            Field<&game::Board::edges>{Keys{"edges"}, Defaults{Value<game::EdgeMode::SOLID>}},
            Field<&game::Board::obstacles>{Keys{"obstacles"}}
        };
    }
};

template<>
struct SchemaTraits<game::Snake> {
    static auto schema() {
        return Object{
            Field<&game::Snake::starting_length>{Keys{"starting_length"}, Defaults{Value<3>}, Constraints{AtLeast<1>}},
            Field<&game::Snake::starting_position>{Keys{"starting_position"}, Defaults{[] { return game::Position{5, 10}; }}},
            Field<&game::Snake::starting_direction>{Keys{"starting_direction"}, Defaults{Value<game::Direction::RIGHT>}},
            Field<&game::Snake::move_interval>{Keys{"move_interval_ms"}, Defaults{[] { return game::Milliseconds{150}; }}, Constraints{test_schemas::PositiveDuration}}
        };
    }
};

template<>
struct SchemaTraits<game::Fruit> {
    static auto schema() {
        return Object{
            Field<&game::Fruit::points>{Keys{"points"}, Defaults{Value<10>}, Constraints{AtLeast<0>}},
            Field<&game::Fruit::growth>{Keys{"growth"}, Defaults{Value<1>}, Constraints{AtLeast<0>}},
            Field<&game::Fruit::spawn_weight>{Keys{"spawn_weight"}, Defaults{Value<1>}, Constraints{AtLeast<1>}},
            Field<&game::Fruit::expires_after>{Keys{"expires_after_ms"}, Constraints{test_schemas::OptionalPositiveDuration}}
        };
    }
};

template<>
struct SchemaTraits<game::Speed> {
    static auto schema() {
        return Object{
            Field<&game::Speed::multiplier>{Keys{"multiplier"}, REQUIRED, Constraints{test_schemas::PositiveMultiplier}},
            Field<&game::Speed::duration>{Keys{"duration_ms"}, REQUIRED, Constraints{test_schemas::PositiveDuration}}
        };
    }
};

template<>
struct SchemaTraits<game::Shield> {
    static auto schema() {
        return Object{
            Field<&game::Shield::charges>{Keys{"charges"}, Defaults{Value<1>}, Constraints{AtLeast<1>}},
            Field<&game::Shield::duration>{Keys{"duration_ms"}, REQUIRED, Constraints{test_schemas::PositiveDuration}}
        };
    }
};

template<>
struct SchemaTraits<game::ScoreMultiplier> {
    static auto schema() {
        return Object{
            Field<&game::ScoreMultiplier::multiplier>{Keys{"multiplier"}, REQUIRED, Constraints{AtLeast<1>}},
            Field<&game::ScoreMultiplier::duration>{Keys{"duration_ms"}, REQUIRED, Constraints{test_schemas::PositiveDuration}}
        };
    }
};

template<>
struct SchemaTraits<game::Effect> {
    static auto schema() {
        return Variant{Bindings{
            Bind<game::EffectType::SPEED, game::Speed>{},
            Bind<game::EffectType::SHIELD, game::Shield>{},
            Bind<game::EffectType::SCORE_MULTIPLIER, game::ScoreMultiplier>{}
        }};
    }
};

template<>
struct SchemaTraits<game::Powerup> {
    static auto schema() {
        return Object{
            Field<&game::Powerup::spawn_weight>{Keys{"spawn_weight"}, Defaults{Value<1>}, Constraints{AtLeast<1>}},
            Field<&game::Powerup::expires_after>{Keys{"expires_after_ms"}, Constraints{test_schemas::OptionalPositiveDuration}},
            Field<&game::Powerup::effect>{Keys{"effect"}, REQUIRED}
        };
    }
};

template<>
struct SchemaTraits<game::Wander> {
    static auto schema() {
        return Object{
            Field<&game::Wander::turn_chance>{Keys{"turn_chance"}, Defaults{Value<0.25>}, Constraints{Range<0.0, 1.0>}}
        };
    }
};

template<>
struct SchemaTraits<game::Chase> {
    static auto schema() {
        return Object{
            Field<&game::Chase::detection_radius>{Keys{"detection_radius"}, Defaults{Value<6>}, Constraints{AtLeast<1>}}
        };
    }
};

template<>
struct SchemaTraits<game::Patrol> {
    static auto schema() {
        return Object{
            Field<&game::Patrol::waypoints>{Keys{"waypoints"}, REQUIRED, Constraints{SizeAtLeast<2>}},
            Field<&game::Patrol::loop>{Keys{"loop"}, Defaults{Value<true>}}
        };
    }
};

template<>
struct SchemaTraits<game::Behavior> {
    static auto schema() {
        return Variant{Bindings{
            Bind<game::BehaviorType::WANDER, game::Wander>{},
            Bind<game::BehaviorType::CHASE, game::Chase>{},
            Bind<game::BehaviorType::PATROL, game::Patrol>{}
        }};
    }
};

template<>
struct SchemaTraits<game::Enemy> {
    static auto schema() {
        return Object{
            Field<&game::Enemy::move_interval>{Keys{"move_interval_ms"}, Defaults{[] { return game::Milliseconds{500}; }}, Constraints{test_schemas::PositiveDuration}},
            Field<&game::Enemy::behavior>{Keys{"behavior"}, REQUIRED}
        };
    }
};

template<>
struct SchemaTraits<game::FruitSpawn> {
    static auto schema() {
        return Object{
            Field<&game::FruitSpawn::max_active>{Keys{"max_active"}, Defaults{Value<1>}, Constraints{AtLeast<0>}},
            Field<&game::FruitSpawn::interval>{Keys{"interval_ms"}, Defaults{[] { return game::Milliseconds{1000}; }}, Constraints{test_schemas::PositiveDuration}}
        };
    }
};

template<>
struct SchemaTraits<game::PowerupSpawn> {
    static auto schema() {
        return Object{
            Field<&game::PowerupSpawn::max_active>{Keys{"max_active"}, Defaults{Value<0>}, Constraints{AtLeast<0>}},
            Field<&game::PowerupSpawn::interval>{Keys{"interval_ms"}, Defaults{[] { return game::Milliseconds{10000}; }}, Constraints{test_schemas::PositiveDuration}}
        };
    }
};

template<>
struct SchemaTraits<game::EnemySpawn> {
    static auto schema() {
        return Object{
            Field<&game::EnemySpawn::max_active>{Keys{"max_active"}, Defaults{Value<0>}, Constraints{AtLeast<0>}},
            Field<&game::EnemySpawn::interval>{Keys{"interval_ms"}, Defaults{[] { return game::Milliseconds{15000}; }}, Constraints{test_schemas::PositiveDuration}}
        };
    }
};

template<>
struct SchemaTraits<game::Spawning> {
    static auto schema() {
        return Object{
            Field<&game::Spawning::fruit>{Keys{"fruit"}},
            Field<&game::Spawning::powerups>{Keys{"powerups"}},
            Field<&game::Spawning::enemies>{Keys{"enemies"}}
        };
    }
};

template<>
struct SchemaTraits<game::Game> {
    static auto schema() {
        return Object{
            Field<&game::Game::name>{Keys{"name"}, Defaults{[] { return std::string{"Snake"}; }}},
            Field<&game::Game::video>{Keys{"video"}},
            Field<&game::Game::audio>{Keys{"audio"}},
            Field<&game::Game::board>{Keys{"board"}},
            Field<&game::Game::snake>{Keys{"snake"}},
            Field<&game::Game::fruits>{Keys{"fruits"}, REQUIRED, Constraints{NotEmpty}},
            Field<&game::Game::powerups>{Keys{"powerups"}},
            Field<&game::Game::enemies>{Keys{"enemies"}},
            Field<&game::Game::spawning>{Keys{"spawning"}}
        };
    }
};

} // namespace struo
