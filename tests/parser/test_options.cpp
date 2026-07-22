#include <doctest/doctest.h>

#include <pjh_cli/app.hpp>
#include <pjh_cli/fixed_string.hpp>
#include "test_helpers.hpp"

TEST_CASE("Parser empty args")
{
    App app("test", "1.0", "Empty test");
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
}

TEST_CASE("Parser simple long option")
{
    App app("test", "1.0", "Long option test");
    app.option<fixed_string("port")>("--port", "Port number").integer();
    Argv argv{"test", "--port", "8080"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto val = r.unwrap().get<int, fixed_string("port")>();
    CHECK(val == 8080);
}

TEST_CASE("Parser long option with equals value")
{
    App app("test", "1.0", "Equals test");
    app.option<fixed_string("port")>("--port", "Port number").integer();
    Argv argv{"test", "--port=8080"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto val = r.unwrap().get<int, fixed_string("port")>();
    CHECK(val == 8080);
}

TEST_CASE("Parser long option with empty equals")
{
    App app("test", "1.0", "Empty equals test");
    app.option<fixed_string("port")>("--port", "Port number").integer();
    Argv argv{"test", "--port="};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser short option next token")
{
    App app("test", "1.0", "Short test");
    app.option<fixed_string("port")>("--port", 'p', "Port").integer();
    Argv argv{"test", "-p", "8080"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto val = r.unwrap().get<int, fixed_string("port")>();
    CHECK(val == 8080);
}

TEST_CASE("Parser multiple options")
{
    App app("test", "1.0", "Multiple test");
    app.option<fixed_string("port")>("--port", "Port").integer();
    app.option<fixed_string("verbose")>("--verbose", "Verbose").boolean();
    Argv argv{"test", "--port", "8080", "--verbose"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto &ctx = r.unwrap();
    auto port = ctx.get<int, fixed_string("port")>();
    CHECK(port == 8080);
    auto verb = ctx.get<bool, fixed_string("verbose")>();
    CHECK(verb == true);
}

TEST_CASE("Parser default applied when option not provided")
{
    App app("test", "1.0", "Default test");
    app.option<fixed_string("port")>("--port", "Port", 3000);
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto val = r.unwrap().get<int, fixed_string("port")>();
    CHECK(val == 3000);
}

TEST_CASE("Parser option value overrides default")
{
    App app("test", "1.0", "Override test");
    app.option<fixed_string("port")>("--port", "Port", 3000);
    Argv argv{"test", "--port", "8080"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto val = r.unwrap().get<int, fixed_string("port")>();
    CHECK(val == 8080);
}

TEST_CASE("Parser equals form type conversion failure")
{
    App app("test", "1.0", "Equals conversion");
    app.option<fixed_string("port")>("--port", "Port").integer();
    Argv argv{"test", "--port=notanumber"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser short mixed bool flag then valued option errors")
{
    App app("test", "1.0", "Mixed short");
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    app.option<fixed_string("port")>("--port", 'p', "Port").integer();
    Argv argv{"test", "-vp", "8080"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser short mixed valued option then bool flag errors")
{
    App app("test", "1.0", "Mixed short");
    app.option<fixed_string("port")>("--port", 'p', "Port").integer();
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    Argv argv{"test", "-pv", "8080"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser short option with attached value errors")
{
    App app("test", "1.0", "Attached test");
    app.option<fixed_string("port")>("--port", 'p', "Port").integer();
    Argv argv{"test", "-p8080"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser short valued option alone works")
{
    App app("test", "1.0", "Separate test");
    app.option<fixed_string("port")>("--port", 'p', "Port").integer();
    Argv argv{"test", "-p", "8080"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto val = r.unwrap().get<int, fixed_string("port")>();
    CHECK(val == 8080);
}

TEST_CASE("Parser short flags group works")
{
    App app("test", "1.0", "Flag group");
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    app.option<fixed_string("force")>("--force", 'f', "Force").boolean();
    Argv argv{"test", "-vf"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.get<bool, fixed_string("verbose")>() == true);
    CHECK(ctx.get<bool, fixed_string("force")>() == true);
}

TEST_CASE("Parser valued option in group middle errors")
{
    App app("test", "1.0", "Group middle");
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").boolean();
    app.option<fixed_string("port")>("--port", 'p', "Port").integer();
    app.option<fixed_string("force")>("--force", 'f', "Force").boolean();
    Argv argv{"test", "-vpf", "8080"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}
