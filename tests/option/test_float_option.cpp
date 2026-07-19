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

TEST_CASE("FloatOption parse_value stores double")
{
    App app("test", "1.0", "Float");
    app.option<fixed_string("rate")>("--rate", "Rate").floating();
    Argv argv{"test", "--rate", "3.14"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<double, fixed_string("rate")>() == doctest::Approx(3.14));
}

TEST_CASE("FloatOption default_value applied when absent")
{
    App app("test", "1.0", "Float default");
    app.option<fixed_string("rate")>("--rate", "Rate").floating().default_value(2.5);
    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    CHECK(r.is_ok());
    CHECK(r.unwrap().get<double, fixed_string("rate")>() == doctest::Approx(2.5));
}

TEST_CASE("FloatOption has_default reflects default_value")
{
    App app("test", "1.0", "Float has_default");
    app.option<fixed_string("rate")>("--rate", "Rate").floating().default_value(1.0);
    auto *def = app.find_option_by_long("rate");
    CHECK(def != nullptr);
    CHECK(def->has_default());
}
