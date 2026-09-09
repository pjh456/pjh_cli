#include <doctest/doctest.h>

#include <iostream>
#include <pjh_cli/app.hpp>
#include <pjh_cli/core/fixed_string.hpp>
#include <string_view>

#include "test_helpers.hpp"

TEST_CASE("Parser bool flag")
{
    App app("test", "1.0", "Flag test");
    app.option<fixed_string("verbose")>("--verbose", "Verbose").boolean();
    Argv argv{"test", "--verbose"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto val = r.unwrap().get<bool, fixed_string("verbose")>();
    CHECK(val == true);
}

TEST_CASE("Parser multiple short flags")
{
    App app("test", "1.0", "Short flags test");
    app.option<fixed_string("a")>("--a", 'a', "Flag A").boolean();
    app.option<fixed_string("b")>("--b", 'b', "Flag B").boolean();
    app.option<fixed_string("c")>("--c", 'c', "Flag C").boolean();
    Argv argv{"test", "-abc"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.get<bool, fixed_string("a")>() == true);
    CHECK(ctx.get<bool, fixed_string("b")>() == true);
    CHECK(ctx.get<bool, fixed_string("c")>() == true);
}

TEST_CASE("Parser bool flag rejects --opt=value")
{
    App app("test", "1.0", "Flag eq");
    app.option<fixed_string("verbose")>("--verbose", "Verbose").boolean();
    Argv argv{"test", "--verbose=false"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
    CHECK(
        r.unwrap_err().what() ==
        std::string_view("Parse Error: option '--verbose' does not accept a value"));
}

TEST_CASE("Parser count flag rejects --opt=value")
{
    App app("test", "1.0", "Count eq");
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbosity").count();
    Argv argv{"test", "--verbose=3"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser count flag rejects --opt=")
{
    App app("test", "1.0", "Count empty eq");
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbosity").count();
    Argv argv{"test", "--verbose="};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser short flag rejects -v=1")
{
    App app("test", "1.0", "Short eq");
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbosity").count();
    Argv argv{"test", "-v=1"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser valued option still accepts --opt=value")
{
    App app("test", "1.0", "Valued eq");
    app.option<fixed_string("port")>("--port", 'p', "Port").integer();
    Argv argv{"test", "--port=8080"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    CHECK(r.unwrap().get<int, fixed_string("port")>() == 8080);
}
