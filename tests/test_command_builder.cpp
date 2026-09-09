#include <doctest/doctest.h>

#include <filesystem>
#include <pjh_cli/command/command_builder.hpp>
#include <pjh_cli/command/leaf_command.hpp>
#include <pjh_cli/core/fixed_string.hpp>
#include <pjh_cli/core/type.hpp>
#include <string>
#include <utility>
#include <vector>

using namespace pjh::cli;

namespace
{
    enum class Mode
    {
        fast,
        slow
    };
}

TEST_CASE("Command builder aggregator instantiates every option kind")
{
    LeafCommand cmd("build", "Builder aggregator self-containment");
    cmd.option<fixed_string("bool-opt")>("--bool-opt", "Boolean flag").boolean();
    cmd.option<fixed_string("count-opt")>("--count-opt", "Counting flag").count();
    cmd.option<fixed_string("enum-opt")>("--enum-opt", "Enum value")
        .enum_type<Mode>()
        .mapping({{"fast", Mode::fast}, {"slow", Mode::slow}});
    cmd.option<fixed_string("float-opt")>("--float-opt", "Float value").floating();
    cmd.option<fixed_string("int-opt")>("--int-opt", "Integer value").integer();
    cmd.option<fixed_string("path-opt")>("--path-opt", "Path value").path();
    cmd.option<fixed_string("str-opt")>("--str-opt", "String value").str();

    CHECK(cmd.options().size() == 7u);
    CHECK(cmd.find_option_by_long("bool-opt") != nullptr);
    CHECK(cmd.find_option_by_long("count-opt") != nullptr);
    CHECK(cmd.find_option_by_long("enum-opt") != nullptr);
    CHECK(cmd.find_option_by_long("float-opt") != nullptr);
    CHECK(cmd.find_option_by_long("int-opt") != nullptr);
    CHECK(cmd.find_option_by_long("path-opt") != nullptr);
    CHECK(cmd.find_option_by_long("str-opt") != nullptr);
}

TEST_CASE("Command builder aggregator resolves the inferred default overload")
{
    LeafCommand cmd("build", "Default overload");
    auto &port = cmd.option<fixed_string("port")>("--port", "Port", 8080);

    CHECK(port.long_name() == "port");
    CHECK(port.has_default());
}

TEST_CASE("Command builder inferred default covers every builtin type")
{
    LeafCommand cmd("build", "Inferred defaults");
    cmd.option<fixed_string("flag")>("--flag", "Bool", true);
    cmd.option<fixed_string("num")>("--num", "Int", 1);
    cmd.option<fixed_string("ratio")>("--ratio", "Double", 1.5);
    cmd.option<fixed_string("name")>("--name", "Str", std::string("s"));
    cmd.option<fixed_string("out")>("--out", "Path", std::filesystem::path("p"));

    REQUIRE(cmd.options().size() == 5u);
    CHECK(cmd.find_option_by_long("flag")->has_default());
    CHECK(cmd.find_option_by_long("num")->has_default());
    CHECK(cmd.find_option_by_long("ratio")->has_default());
    CHECK(cmd.find_option_by_long("name")->has_default());
    CHECK(cmd.find_option_by_long("out")->has_default());
}

TEST_CASE("Command builder aggregator resolves option groups")
{
    LeafCommand cmd("build", "Option group");
    cmd.option<fixed_string("port")>("--port", "Port").integer();
    cmd.option<fixed_string("socket")>("--socket", "Socket").str();
    cmd.group<fixed_string("port"), fixed_string("socket")>().exactly_one();

    REQUIRE(cmd.groups().size() == 1u);
    CHECK(cmd.groups().front().mode == GroupMode::ExactlyOne);
    CHECK(cmd.groups().front().option_names.size() == 2u);
}
