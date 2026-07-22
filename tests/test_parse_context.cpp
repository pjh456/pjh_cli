#include <doctest/doctest.h>

#include <initializer_list>
#include <memory>
#include <pjh_cli/app.hpp>
#include <pjh_cli/core/fixed_string.hpp>
#include <pjh_cli/parse/parse_context.hpp>
#include <string>
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

// ── Parent chain tests ──

TEST_CASE("parent chain has_value falls back to parent")
{
    auto parent = std::make_shared<ParseContext>();
    parent->set_value<int>(42, 100);

    ParseContext child;
    child.set_parent(parent);

    CHECK(child.has_value(42));
}

TEST_CASE("parent chain has_value prefers own value over parent")
{
    auto parent = std::make_shared<ParseContext>();
    parent->set_value<int>(42, 100);

    ParseContext child;
    child.set_parent(parent);
    child.set_value<int>(42, 200);

    CHECK(child.get_value<int>(42, -1) == 200);
}

TEST_CASE("parent chain get_value falls back to parent")
{
    auto parent = std::make_shared<ParseContext>();
    parent->set_value<int>(42, 100);

    ParseContext child;
    child.set_parent(parent);

    CHECK(child.get_value<int>(42, -1) == 100);
}

TEST_CASE("parent chain get_value returns own value when both set")
{
    auto parent = std::make_shared<ParseContext>();
    parent->set_value<int>(42, 100);

    ParseContext child;
    child.set_parent(parent);
    child.set_value<int>(42, 200);

    CHECK(child.get_value<int>(42, -1) == 200);
}

TEST_CASE("parent chain has_value returns false when neither has value")
{
    auto parent = std::make_shared<ParseContext>();
    ParseContext child;
    child.set_parent(parent);

    CHECK_FALSE(child.has_value(99));
}

TEST_CASE("parent chain has<Key>() falls back to parent")
{
    auto parent = std::make_shared<ParseContext>();
    constexpr auto h = key_hash(fixed_string("verbose"));
    parent->set_value<bool>(h, true);

    ParseContext child;
    child.set_parent(parent);

    CHECK(child.has<fixed_string("verbose")>());
}

TEST_CASE("parent chain get<T,Key>() falls back to parent")
{
    auto parent = std::make_shared<ParseContext>();
    constexpr auto h = key_hash(fixed_string("port"));
    parent->set_value<int>(h, 8080);

    ParseContext child;
    child.set_parent(parent);

    CHECK(child.get<int, fixed_string("port")>() == 8080);
}

TEST_CASE("parent chain get<T,Key>() prefers own value")
{
    auto parent = std::make_shared<ParseContext>();
    constexpr auto h_p = key_hash(fixed_string("port"));
    parent->set_value<int>(h_p, 8080);

    ParseContext child;
    child.set_parent(parent);
    constexpr auto h_c = key_hash(fixed_string("port"));
    child.set_value<int>(h_c, 9090);

    CHECK(child.get<int, fixed_string("port")>() == 9090);
}

TEST_CASE("parent chain try_get falls back to parent")
{
    auto parent = std::make_shared<ParseContext>();
    constexpr auto h = key_hash(fixed_string("verbose"));
    parent->set_value<bool>(h, true);

    ParseContext child;
    child.set_parent(parent);

    auto r = child.try_get<bool, fixed_string("verbose")>();
    CHECK(r.is_some());
    CHECK(r.unwrap() == true);
}

TEST_CASE("parent chain try_get returns None when neither has value")
{
    auto parent = std::make_shared<ParseContext>();
    ParseContext child;
    child.set_parent(parent);

    CHECK_FALSE(child.try_get<int, fixed_string("missing")>().is_some());
}

TEST_CASE("parent chain get_or falls back to parent")
{
    auto parent = std::make_shared<ParseContext>();
    constexpr auto h = key_hash(fixed_string("port"));
    parent->set_value<int>(h, 8080);

    ParseContext child;
    child.set_parent(parent);

    CHECK(child.get_or<int, fixed_string("port")>(999) == 8080);
}

TEST_CASE("parent chain get_or returns fallback when neither has value")
{
    auto parent = std::make_shared<ParseContext>();
    ParseContext child;
    child.set_parent(parent);

    CHECK(child.get_or<int, fixed_string("port")>(999) == 999);
}

TEST_CASE("parent chain own absent value falls back to parent try_get")
{
    auto parent = std::make_shared<ParseContext>();
    constexpr auto h = key_hash(fixed_string("name"));
    parent->set_value<std::string>(h, std::string("parent"));

    ParseContext child;
    child.set_parent(parent);
    // Child has its own "port" but not "name"
    constexpr auto h2 = key_hash(fixed_string("port"));
    child.set_value<int>(h2, 111);

    auto r = child.try_get<std::string, fixed_string("name")>();
    CHECK(r.is_some());
    CHECK(r.unwrap() == "parent");
}

TEST_CASE("parent chain deep nesting all levels visible")
{
    auto root = std::make_shared<ParseContext>();
    constexpr auto h1 = key_hash(fixed_string("level1"));
    root->set_value<int>(h1, 1);

    auto mid = std::make_shared<ParseContext>();
    constexpr auto h2 = key_hash(fixed_string("level2"));
    mid->set_value<int>(h2, 2);
    mid->set_parent(root);

    ParseContext leaf;
    constexpr auto h3 = key_hash(fixed_string("level3"));
    leaf.set_value<int>(h3, 3);
    leaf.set_parent(mid);

    CHECK(leaf.get<int, fixed_string("level3")>() == 3);
    CHECK(leaf.get<int, fixed_string("level2")>() == 2);
    CHECK(leaf.get<int, fixed_string("level1")>() == 1);
}
