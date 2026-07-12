#include <doctest/doctest.h>
#include "test_helpers.hpp"

TEST_CASE("Parser positional args")
{
    App app("test", "1.0", "Positional test");
    app.arg<std::string, 0>("source", "Source file");
    app.arg<std::string, 1>("dest", "Destination");
    Argv argv{"test", "in.txt", "out.txt"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.get<std::string, 0>() == "in.txt");
    CHECK(ctx.get<std::string, 1>() == "out.txt");
}

TEST_CASE("Parser mixed options and positional args")
{
    App app("test", "1.0", "Mixed test");
    app.option<bool, fixed_string("verbose")>("--verbose", 'v', "Verbose");
    app.arg<std::string, 0>("file", "Input file");
    Argv argv{"test", "--verbose", "data.txt"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.get<bool, fixed_string("verbose")>() == true);
    CHECK(ctx.get<std::string, 0>() == "data.txt");
}

TEST_CASE("Parser required arg insufficient args")
{
    App app("test", "1.0", "Insufficient required");
    app.arg<std::string, 0>("src", "Source").required();
    app.arg<std::string, 1>("dst", "Dest").required();
    Argv argv{"test", "onlysrc"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser optional arg insufficient does not fail")
{
    App app("test", "1.0", "Insufficient optional");
    app.arg<std::string, 0>("src", "Source");
    app.arg<std::string, 1>("dst", "Dest");
    Argv argv{"test", "onlysrc"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
}

TEST_CASE("Parser all positional store with no registered args")
{
    App app("test", "1.0", "All positional store");
    app.set_extra_args(ExtraArgsPolicy::Store);
    Argv argv{"test", "a.txt", "b.txt", "c.txt"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto extra = r.unwrap().extra_args();
    CHECK(extra.size() == 3);
    CHECK(extra[0] == "a.txt");
    CHECK(extra[1] == "b.txt");
    CHECK(extra[2] == "c.txt");
}

TEST_CASE("Parser all positional ignore with no registered args")
{
    App app("test", "1.0", "All positional ignore");
    Argv argv{"test", "a.txt", "b.txt"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
}

TEST_CASE("Parser all positional error with no registered args")
{
    App app("test", "1.0", "All positional error");
    app.set_extra_args(ExtraArgsPolicy::Error);
    Argv argv{"test", "a.txt"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser extra positional args ignore")
{
    App app("test", "1.0", "Extra args test");
    app.arg<std::string, 0>("file", "Input file");
    Argv argv{"test", "a.txt", "b.txt", "c.txt"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<std::string, 0>() == "a.txt");
}

TEST_CASE("Parser extra positional args error policy")
{
    App app("test", "1.0", "Strict error test");
    app.set_extra_args(ExtraArgsPolicy::Error);
    app.arg<std::string, 0>("file", "Input file");
    Argv argv{"test", "a.txt", "b.txt"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_err());
}

TEST_CASE("Parser extra positional args store policy")
{
    App app("test", "1.0", "Store test");
    app.set_extra_args(ExtraArgsPolicy::Store);
    app.arg<std::string, 0>("file", "Input file");
    Argv argv{"test", "a.txt", "b.txt", "c.txt"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.get<std::string, 0>() == "a.txt");
    auto extra = ctx.extra_args();
    CHECK(extra.size() == 2);
    CHECK(extra[0] == "b.txt");
    CHECK(extra[1] == "c.txt");
}

TEST_CASE("Parser extra positional args store no extra")
{
    App app("test", "1.0", "Store no extra");
    app.set_extra_args(ExtraArgsPolicy::Store);
    app.arg<std::string, 0>("file", "Input file");
    Argv argv{"test", "a.txt"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().extra_args().empty());
}
