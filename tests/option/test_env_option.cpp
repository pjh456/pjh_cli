#include <doctest/doctest.h>

#include <cstdlib>
#include <initializer_list>
#include <pjh_cli/app.hpp>
#include <pjh_cli/core/fixed_string.hpp>
#include <pjh_cli/parse/parser.hpp>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "test_helpers.hpp"

using namespace pjh::cli;

static_assert(
    noexcept(std::declval<const detail::EnvSnapshot &>().get(std::string_view{})),
    "EnvSnapshot::get is an allocation-free heterogeneous lookup; must stay noexcept");

struct Argv
{
    std::vector<std::string> storage;
    std::vector<char *> ptrs;
    Argv(std::initializer_list<std::string> list) : storage(list)
    {
        for (auto &s : storage) ptrs.push_back(s.data());
    }
    int argc() const { return static_cast<int>(ptrs.size()); }
    char **argv() { return ptrs.data(); }
};

TEST_CASE("EnvVar int option reads from environment")
{
    ScopedEnvVar env_guard{"PJH_CLI_TEST_ENV_PORT", "8080"};
    App app("test", "1.0", "Env int");
    app.option<fixed_string("port")>("--port", "Port").integer().env(env_guard.name());
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<int, fixed_string("port")>() == 8080);
}

TEST_CASE("EnvVar string option reads from environment")
{
    ScopedEnvVar env_guard{"PJH_CLI_TEST_ENV_HOST", "example.com"};
    App app("test", "1.0", "Env str");
    app.option<fixed_string("host")>("--host", "Host").str().env(env_guard.name());
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<std::string, fixed_string("host")>() == "example.com");
}

TEST_CASE("EnvVar CLI value overrides environment")
{
    ScopedEnvVar env_guard{"PJH_CLI_TEST_ENV_PORT", "3000"};
    App app("test", "1.0", "Env override");
    app.option<fixed_string("port")>("--port", "Port").integer().env(env_guard.name());
    Argv argv{"test", "--port", "8080"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<int, fixed_string("port")>() == 8080);
}

TEST_CASE("EnvVar default falls through to hardcoded default")
{
    ScopedEnvVar env_guard{"PJH_CLI_TEST_ENV_PORT", std::nullopt};
    App app("test", "1.0", "Env default");
    app.option<fixed_string("port")>("--port", "Port")
        .integer()
        .default_value(9999)
        .env(env_guard.name());
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<int, fixed_string("port")>() == 9999);
}

TEST_CASE("EnvVar satisfies required")
{
    ScopedEnvVar env_guard{"PJH_CLI_TEST_ENV_TOKEN", "secret123"};
    App app("test", "1.0", "Env required");
    app.option<fixed_string("token")>("--token", "Token")
        .str()
        .env(env_guard.name())
        .required();
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<std::string, fixed_string("token")>() == "secret123");
}

TEST_CASE("EnvVar absent env var does not set value")
{
    ScopedEnvVar env_guard{"PJH_CLI_TEST_ENV_PORT", std::nullopt};
    App app("test", "1.0", "Env absent");
    app.option<fixed_string("port")>("--port", "Port").integer().env(env_guard.name());
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK_FALSE(r.unwrap().has<fixed_string("port")>());
}

TEST_CASE("EnvVar env overrides default when both set")
{
    ScopedEnvVar env_guard{"PJH_CLI_TEST_ENV_PORT", "3000"};
    App app("test", "1.0", "Env over default");
    app.option<fixed_string("port")>("--port", "Port")
        .integer()
        .default_value(5000)
        .env(env_guard.name());
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<int, fixed_string("port")>() == 3000);
}

TEST_CASE("EnvVar bool option reads true from environment")
{
    ScopedEnvVar env_guard{"PJH_CLI_TEST_ENV_VERBOSE", "true"};
    App app("test", "1.0", "Env bool true");
    app.option<fixed_string("verbose")>("--verbose", "Verbose")
        .boolean()
        .env(env_guard.name());
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<bool, fixed_string("verbose")>() == true);
}

TEST_CASE("EnvVar bool option reads false from environment")
{
    ScopedEnvVar env_guard{"PJH_CLI_TEST_ENV_VERBOSE", "0"};
    App app("test", "1.0", "Env bool false");
    app.option<fixed_string("verbose")>("--verbose", "Verbose")
        .boolean()
        .env(env_guard.name());
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<bool, fixed_string("verbose")>() == false);
}

TEST_CASE("EnvVar bool option overrides default")
{
    ScopedEnvVar env_guard{"PJH_CLI_TEST_ENV_VERBOSE", "true"};
    App app("test", "1.0", "Env bool over default");
    app.option<fixed_string("verbose")>("--verbose", "Verbose")
        .boolean()
        .default_value(false)
        .env(env_guard.name());
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<bool, fixed_string("verbose")>() == true);
}

TEST_CASE("EnvVar bool CLI flag overrides environment")
{
    ScopedEnvVar env_guard{"PJH_CLI_TEST_ENV_VERBOSE", "false"};
    App app("test", "1.0", "Env bool CLI override");
    app.option<fixed_string("verbose")>("--verbose", "Verbose")
        .boolean()
        .env(env_guard.name());
    Argv argv{"test", "--verbose"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<bool, fixed_string("verbose")>() == true);
}

TEST_CASE("EnvVar bool invalid value errors")
{
    ScopedEnvVar env_guard{"PJH_CLI_TEST_ENV_VERBOSE", "maybe"};
    App app("test", "1.0", "Env bool invalid");
    app.option<fixed_string("verbose")>("--verbose", "Verbose")
        .boolean()
        .env(env_guard.name());
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
    auto msg = std::string_view(r.unwrap_err().what());
    CHECK(msg.find("invalid value 'maybe' for '--verbose'") != std::string_view::npos);
    CHECK(msg.find("expected bool") != std::string_view::npos);
}

TEST_CASE("EnvVar option on subcommand reads environment")
{
    ScopedEnvVar env_guard{"PJH_CLI_TEST_ENV_HOST", "sub.example.com"};
    App app("test", "1.0", "Env subcommand");
    app.add_leaf("sub", "Sub")
        .option<fixed_string("host")>("--host", "Host")
        .str()
        .env(env_guard.name());
    Argv argv{"test", "sub"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<std::string, fixed_string("host")>() == "sub.example.com");
}

TEST_CASE("ScopedEnvVar restores previous value")
{
    ScopedEnvVar outer{"PJH_CLI_TEST_ENV_RESTORE", "before"};
    {
        ScopedEnvVar inner{"PJH_CLI_TEST_ENV_RESTORE", "during"};
        const char *inner_value = std::getenv("PJH_CLI_TEST_ENV_RESTORE");
        REQUIRE(inner_value != nullptr);
        CHECK(std::string(inner_value) == "during");
    }
    const char *restored_value = std::getenv("PJH_CLI_TEST_ENV_RESTORE");
    REQUIRE(restored_value != nullptr);
    CHECK(std::string(restored_value) == "before");
}

TEST_CASE("ScopedEnvVar removes variable when value is nullopt")
{
    ScopedEnvVar outer{"PJH_CLI_TEST_ENV_REMOVE", "x"};
    {
        ScopedEnvVar inner{"PJH_CLI_TEST_ENV_REMOVE", std::nullopt};
        CHECK(std::getenv("PJH_CLI_TEST_ENV_REMOVE") == nullptr);
    }
    const char *restored_value = std::getenv("PJH_CLI_TEST_ENV_REMOVE");
    REQUIRE(restored_value != nullptr);
    CHECK(std::string(restored_value) == "x");
}

TEST_CASE("env fallback resolves through the root command's env_value")
{
    ScopedEnvVar env_guard{"PJH_CLI_TEST_ENV_SEAM", "seam-value"};
    App app("test", "1.0", "Seam");
    app.option<fixed_string("seam")>("--seam", "Seam").str().env(env_guard.name());
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<std::string, fixed_string("seam")>() == "seam-value");
}

TEST_CASE("bare BranchCommand root has no env fallback")
{
    ScopedEnvVar env_guard{"PJH_CLI_TEST_ENV_SEAM", "seam-value"};
    BranchCommand root("bare", "Bare root");
    root.option<fixed_string("seam")>("--seam", "Seam").str().env(env_guard.name());
    std::span<const std::string_view> args{};
    auto r = Parser::parse_command(root, args, 0);
    CHECK(r.is_ok());
    CHECK_FALSE(r.unwrap().has<fixed_string("seam")>());
}

TEST_CASE("EnvSnapshot get resolves present and absent names directly")
{
    ScopedEnvVar env_guard{"PJH_CLI_TEST_ENV_DIRECT", "direct-value"};
    detail::EnvSnapshot snap;
    auto *value = snap.get(env_guard.name());
    REQUIRE(value != nullptr);
    CHECK(*value == "direct-value");
    CHECK(snap.get("PJH_CLI_TEST_ENV_NEVER_SET") == nullptr);
}

#ifdef _WIN32
TEST_CASE("EnvSnapshot reads Windows environment")
{
    ScopedEnvVar env_guard{"PJH_CLI_TEST_ENV_SNAPSHOT", "42"};
    detail::EnvSnapshot snap;
    auto *v = snap.get(env_guard.name());
    REQUIRE(v != nullptr);
    CHECK(*v == "42");
}
#endif

TEST_CASE("EnvVar int option accepts leading plus from environment")
{
    ScopedEnvVar env_guard{"PJH_CLI_TEST_ENV_PLUS_PORT", "+8080"};
    App app("test", "1.0", "Env plus int");
    app.option<fixed_string("port")>("--port", "Port").integer().env(env_guard.name());
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    CHECK(r.unwrap().get<int, fixed_string("port")>() == 8080);
}

TEST_CASE("EnvVar float option rejects nan from environment")
{
    ScopedEnvVar env_guard{"PJH_CLI_TEST_ENV_NAN_RATE", "nan"};
    App app("test", "1.0", "Env nan float");
    app.option<fixed_string("rate")>("--rate", "Rate").floating().env(env_guard.name());
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_err());
    auto msg = std::string_view(r.unwrap_err().what());
    CHECK(msg.find("invalid value 'nan' for '--rate'") != std::string_view::npos);
    CHECK(msg.find("expected float") != std::string_view::npos);
}

TEST_CASE("EnvVar float option rejects inf from environment")
{
    ScopedEnvVar env_guard{"PJH_CLI_TEST_ENV_INF_RATE", "inf"};
    App app("test", "1.0", "Env inf float");
    app.option<fixed_string("rate")>("--rate", "Rate").floating().env(env_guard.name());
    Argv argv{"test"};
    CHECK(app.parse(argv.argc(), argv.argv()).is_err());
}
