#include <doctest/doctest.h>

#include <pjh_cli.hpp>
#include <string>
#include <string_view>
#include <vector>

using namespace pjh::cli;

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

TEST_CASE("CountOption short group increments")
{
    App app("test", "1.0", "Count");
    app.option<fixed_string("v")>("--verbose", 'v', "Verbosity").count();
    Argv argv{"test", "-vvv"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<int, fixed_string("v")>() == 3);
}

TEST_CASE("CountOption separate shorts add up")
{
    App app("test", "1.0", "Count");
    app.option<fixed_string("v")>("--verbose", 'v', "Verbosity").count();
    Argv argv{"test", "-v", "-v"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<int, fixed_string("v")>() == 2);
}

TEST_CASE("CountOption long form increments")
{
    App app("test", "1.0", "Count");
    app.option<fixed_string("v")>("--verbose", 'v', "Verbosity").count();
    Argv argv{"test", "--verbose", "--verbose", "--verbose"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<int, fixed_string("v")>() == 3);
}

TEST_CASE("CountOption absent means no value")
{
    App app("test", "1.0", "Count");
    app.option<fixed_string("v")>("--verbose", 'v', "Verbosity").count();
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK_FALSE(r.unwrap().has<fixed_string("v")>());
}

TEST_CASE("CountOption mixed with bool flags")
{
    App app("test", "1.0", "Count mixed");
    app.option<fixed_string("v")>("--verbose", 'v', "Verbosity").count();
    app.option<fixed_string("f")>("--flag", 'f', "Flag").boolean();
    Argv argv{"test", "-vff"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.get<int, fixed_string("v")>() == 1);
    CHECK(ctx.get<bool, fixed_string("f")>() == true);
}

TEST_CASE("CountOption ignores --opt=value")
{
    App app("test", "1.0", "Count eq");
    app.option<fixed_string("v")>("--verbose", 'v', "Verbosity").count();
    Argv argv{"test", "--verbose=x"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<int, fixed_string("v")>() == 1);
}

TEST_CASE("CountOption no default_value")
{
    // Verify that CountOption doesn't expose default_value
    // (compile test — uncommenting should fail)
    // App app("test", "1.0", "No default");
    // app.option<fixed_string("v")>("--verbose", 'v',
    // "Verbosity").count().default_value(3); ^ expected: compile error (deleted function)
}

TEST_CASE("CountOption has_value false")
{
    App app("test", "1.0", "Has value");
    app.option<fixed_string("v")>("--verbose", 'v', "Verbosity").count();
    auto &cmd = app;
    // CountOption should report has_value=false
    auto *def = cmd.find_option_by_long("verbose");
    CHECK(def != nullptr);
    CHECK_FALSE(def->has_value());
}
