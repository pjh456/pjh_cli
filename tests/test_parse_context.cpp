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

TEST_CASE("try_get returns Some when value present")
{
    App app("test", "1.0", "try_get");
    app.option<fixed_string("port")>("--port", "Port", 8080);
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto tg = r.unwrap().try_get<int, fixed_string("port")>();
    CHECK(tg.is_some());
    CHECK(tg.unwrap() == 8080);
}

TEST_CASE("try_get returns None when value absent")
{
    App app("test", "1.0", "try_get absent");
    app.option<fixed_string("port")>("--port", "Port").integer();
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    auto tg = r.unwrap().try_get<int, fixed_string("port")>();
    CHECK_FALSE(tg.is_some());
}

TEST_CASE("get_or returns value when present")
{
    App app("test", "1.0", "get_or");
    app.option<fixed_string("port")>("--port", "Port", 8080);
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get_or<int, fixed_string("port")>(999) == 8080);
}

TEST_CASE("get_or returns fallback when absent")
{
    App app("test", "1.0", "get_or fallback");
    app.option<fixed_string("port")>("--port", "Port").integer();
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get_or<int, fixed_string("port")>(999) == 999);
}
