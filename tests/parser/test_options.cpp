#include <doctest/doctest.h>
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
    app.option<int, fixed_string("port")>("--port", "Port number");
    Argv argv{"test", "--port", "8080"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto val = r.unwrap().get<int, fixed_string("port")>();
    CHECK(val == 8080);
}

TEST_CASE("Parser long option with equals value")
{
    App app("test", "1.0", "Equals test");
    app.option<int, fixed_string("port")>("--port", "Port number");
    Argv argv{"test", "--port=8080"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto val = r.unwrap().get<int, fixed_string("port")>();
    CHECK(val == 8080);
}

TEST_CASE("Parser long option with empty equals")
{
    App app("test", "1.0", "Empty equals test");
    app.option<int, fixed_string("port")>("--port", "Port number");
    Argv argv{"test", "--port="};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser short option next token")
{
    App app("test", "1.0", "Short test");
    app.option<int, fixed_string("port")>("--port", 'p', "Port");
    Argv argv{"test", "-p", "8080"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto val = r.unwrap().get<int, fixed_string("port")>();
    CHECK(val == 8080);
}

TEST_CASE("Parser multiple options")
{
    App app("test", "1.0", "Multiple test");
    app.option<int, fixed_string("port")>("--port", "Port");
    app.option<bool, fixed_string("verbose")>("--verbose", "Verbose");
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
    app.option<int, fixed_string("port")>("--port", "Port", 3000);
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto val = r.unwrap().get<int, fixed_string("port")>();
    CHECK(val == 3000);
}

TEST_CASE("Parser option value overrides default")
{
    App app("test", "1.0", "Override test");
    app.option<int, fixed_string("port")>("--port", "Port", 3000);
    Argv argv{"test", "--port", "8080"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto val = r.unwrap().get<int, fixed_string("port")>();
    CHECK(val == 8080);
}

TEST_CASE("Parser equals form type conversion failure")
{
    App app("test", "1.0", "Equals conversion");
    app.option<int, fixed_string("port")>("--port", "Port");
    Argv argv{"test", "--port=notanumber"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser short mixed bool flag then valued option errors")
{
    App app("test", "1.0", "Mixed short");
    app.option<bool, fixed_string("verbose")>("--verbose", 'v', "Verbose");
    app.option<int, fixed_string("port")>("--port", 'p', "Port");
    Argv argv{"test", "-vp", "8080"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser short mixed valued option then bool flag errors")
{
    App app("test", "1.0", "Mixed short");
    app.option<int, fixed_string("port")>("--port", 'p', "Port");
    app.option<bool, fixed_string("verbose")>("--verbose", 'v', "Verbose");
    Argv argv{"test", "-pv", "8080"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser short option with attached value errors")
{
    App app("test", "1.0", "Attached test");
    app.option<int, fixed_string("port")>("--port", 'p', "Port");
    Argv argv{"test", "-p8080"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser short valued option alone works")
{
    App app("test", "1.0", "Separate test");
    app.option<int, fixed_string("port")>("--port", 'p', "Port");
    Argv argv{"test", "-p", "8080"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto val = r.unwrap().get<int, fixed_string("port")>();
    CHECK(val == 8080);
}

TEST_CASE("Parser short flags group works")
{
    App app("test", "1.0", "Flag group");
    app.option<bool, fixed_string("verbose")>("--verbose", 'v', "Verbose");
    app.option<bool, fixed_string("force")>("--force", 'f', "Force");
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
    app.option<bool, fixed_string("verbose")>("--verbose", 'v', "Verbose");
    app.option<int, fixed_string("port")>("--port", 'p', "Port");
    app.option<bool, fixed_string("force")>("--force", 'f', "Force");
    Argv argv{"test", "-vpf", "8080"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}
