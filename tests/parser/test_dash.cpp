#include <doctest/doctest.h>

#include "test_helpers.hpp"

TEST_CASE("Parser double dash separator")
{
    App app("test", "1.0", "Double dash test");
    app.option<bool, fixed_string("verbose")>("--verbose", 'v', "Verbose");
    app.arg<std::string, 0>("file", "Input file");
    Argv argv{"test", "--verbose", "--", "--file=foo"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.get<bool, fixed_string("verbose")>() == true);
    CHECK(ctx.get<std::string, 0>() == "--file=foo");
}

TEST_CASE("Parser double dash with no further args")
{
    App app("test", "1.0", "DD empty");
    app.option<bool, fixed_string("verbose")>("--verbose", 'v', "Verbose");
    Argv argv{"test", "--verbose", "--"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<bool, fixed_string("verbose")>() == true);
}

TEST_CASE("Parser double dash with ExtraArgsPolicy::Error")
{
    App app("test", "1.0", "DD error");
    app.set_extra_args(ExtraArgsPolicy::Error);
    app.arg<std::string, 0>("file", "Input file");
    Argv argv{"test", "--", "a.txt", "b.txt"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}
