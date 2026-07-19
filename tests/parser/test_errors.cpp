#include <doctest/doctest.h>

#include "test_helpers.hpp"

TEST_CASE("Parser unknown long option")
{
    App app("test", "1.0", "Unknown test");
    Argv argv{"test", "--bogus"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser unknown short option")
{
    App app("test", "1.0", "Unknown short test");
    Argv argv{"test", "-x"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser missing value for long option")
{
    App app("test", "1.0", "Missing value test");
    app.option<fixed_string("port")>("--port", "Port").integer();
    Argv argv{"test", "--port"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser missing value for short option")
{
    App app("test", "1.0", "Missing short value test");
    app.option<fixed_string("port")>("--port", 'p', "Port").integer();
    Argv argv{"test", "-p"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser required option missing")
{
    App app("test", "1.0", "Required opt test");
    app.option<fixed_string("port")>("--port", "Port").integer().required();
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser required positional arg missing")
{
    App app("test", "1.0", "Required arg test");
    app.arg<std::string, 0>("file", "Input file").required();
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser type conversion failure")
{
    App app("test", "1.0", "Conversion test");
    app.option<fixed_string("port")>("--port", "Port").integer();
    Argv argv{"test", "--port", "notanumber"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser required option and required arg both missing")
{
    App app("test", "1.0", "Both required missing");
    app.option<fixed_string("port")>("--port", "Port").integer().required();
    app.arg<std::string, 0>("file", "Input file").required();
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser error message unknown long option")
{
    App app("test", "1.0", "Err msg");
    Argv argv{"test", "--bogus"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
    CHECK(
        r.unwrap_err().what() ==
        std::string_view("Parse Error: unknown option: '--bogus'"));
}

TEST_CASE("Parser error message unknown short option")
{
    App app("test", "1.0", "Err msg");
    Argv argv{"test", "-x"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
    CHECK(r.unwrap_err().what() == std::string_view("Parse Error: unknown option: '-x'"));
}

TEST_CASE("Parser error message missing value for long option")
{
    App app("test", "1.0", "Err msg");
    app.option<fixed_string("port")>("--port", "Port").integer();
    Argv argv{"test", "--port"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
    CHECK(
        r.unwrap_err().what() ==
        std::string_view("Parse Error: option '--port' requires a value"));
}

TEST_CASE("Parser error message missing value for short option")
{
    App app("test", "1.0", "Err msg");
    app.option<fixed_string("port")>("--port", 'p', "Port").integer();
    Argv argv{"test", "-p"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
    CHECK(
        r.unwrap_err().what() ==
        std::string_view("Parse Error: option '-p' requires a value"));
}

TEST_CASE("Parser error message required option missing")
{
    App app("test", "1.0", "Err msg");
    app.option<fixed_string("port")>("--port", "Port").integer().required();
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
    CHECK(
        r.unwrap_err().what() ==
        std::string_view("Parse Error: missing required option: 'port'"));
}

TEST_CASE("Parser error message required arg missing")
{
    App app("test", "1.0", "Err msg");
    app.arg<std::string, 0>("file", "Input file").required();
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
    CHECK(
        r.unwrap_err().what() ==
        std::string_view("Parse Error: missing required argument: 'file'"));
}

TEST_CASE("Parser error message type conversion failure")
{
    App app("test", "1.0", "Err msg");
    app.option<fixed_string("port")>("--port", "Port").integer();
    Argv argv{"test", "--port", "notanumber"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
    CHECK(
        r.unwrap_err().what() ==
        std::string_view("Parse Error: invalid integer: 'notanumber'"));
}

TEST_CASE("Parser error message disabled command")
{
    App app("test", "1.0", "Err msg");
    app.add_command("oldcmd", "Deprecated").enabled([] { return false; });
    Argv argv{"test", "oldcmd"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
    CHECK(
        r.unwrap_err().what() ==
        std::string_view("Parse Error: command 'oldcmd' is not available"));
}
