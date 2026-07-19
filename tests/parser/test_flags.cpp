#include <doctest/doctest.h>

#include "test_helpers.hpp"

TEST_CASE("Parser bool flag")
{
    App app("test", "1.0", "Flag test");
    app.option<bool, fixed_string("verbose")>("--verbose", "Verbose");
    Argv argv{"test", "--verbose"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto val = r.unwrap().get<bool, fixed_string("verbose")>();
    CHECK(val == true);
}

TEST_CASE("Parser multiple short flags")
{
    App app("test", "1.0", "Short flags test");
    app.option<bool, fixed_string("a")>("--a", 'a', "Flag A");
    app.option<bool, fixed_string("b")>("--b", 'b', "Flag B");
    app.option<bool, fixed_string("c")>("--c", 'c', "Flag C");
    Argv argv{"test", "-abc"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.get<bool, fixed_string("a")>() == true);
    CHECK(ctx.get<bool, fixed_string("b")>() == true);
    CHECK(ctx.get<bool, fixed_string("c")>() == true);
}
