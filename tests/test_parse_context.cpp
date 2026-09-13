#include <doctest/doctest.h>

#include <filesystem>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <pjh_cli/app.hpp>
#include <pjh_cli/core/fixed_string.hpp>
#include <pjh_cli/parse/detail/parse_context_writer.hpp>
#include <pjh_cli/parse/parse_context.hpp>
#include <string>
#include <type_traits>
#include <vector>

using namespace pjh::cli;

// Contract pin: the writer is an implementation detail, not pjh::cli API.
static_assert(
    std::is_same_v<
        decltype(&detail::ParseContextWriter::has_value),
        bool (*)(const ParseContext &, size_t) noexcept>,
    "ParseContextWriter must live in pjh::cli::detail");

// Special-member pin: a user-declared destructor must not suppress the move
// operations, or parsing (which moves ParseContext on every descent) would
// silently deep-copy.  The destructor must stay non-trivial and non-throwing.
static_assert(std::is_nothrow_move_constructible_v<ParseContext>);
static_assert(std::is_move_assignable_v<ParseContext>);
static_assert(std::is_copy_constructible_v<ParseContext>);
static_assert(!std::is_trivially_destructible_v<ParseContext>);
static_assert(std::is_nothrow_destructible_v<ParseContext>);

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
    detail::ParseContextWriter::set_value<int>(*parent, 42, 100);

    ParseContext child;
    detail::ParseContextWriter::set_parent(child, parent);

    CHECK(detail::ParseContextWriter::has_value(child, 42));
}

TEST_CASE("parent chain has_value prefers own value over parent")
{
    auto parent = std::make_shared<ParseContext>();
    detail::ParseContextWriter::set_value<int>(*parent, 42, 100);

    ParseContext child;
    detail::ParseContextWriter::set_parent(child, parent);
    detail::ParseContextWriter::set_value<int>(child, 42, 200);

    CHECK(detail::ParseContextWriter::get_value<int>(child, 42, -1) == 200);
}

TEST_CASE("parent chain get_value falls back to parent")
{
    auto parent = std::make_shared<ParseContext>();
    detail::ParseContextWriter::set_value<int>(*parent, 42, 100);

    ParseContext child;
    detail::ParseContextWriter::set_parent(child, parent);

    CHECK(detail::ParseContextWriter::get_value<int>(child, 42, -1) == 100);
}

TEST_CASE("parent chain get_value returns own value when both set")
{
    auto parent = std::make_shared<ParseContext>();
    detail::ParseContextWriter::set_value<int>(*parent, 42, 100);

    ParseContext child;
    detail::ParseContextWriter::set_parent(child, parent);
    detail::ParseContextWriter::set_value<int>(child, 42, 200);

    CHECK(detail::ParseContextWriter::get_value<int>(child, 42, -1) == 200);
}

TEST_CASE("parent chain has_value returns false when neither has value")
{
    auto parent = std::make_shared<ParseContext>();
    ParseContext child;
    detail::ParseContextWriter::set_parent(child, parent);

    CHECK_FALSE(detail::ParseContextWriter::has_value(child, 99));
}

TEST_CASE("parent chain has<Key>() falls back to parent")
{
    auto parent = std::make_shared<ParseContext>();
    constexpr auto h = key_hash(fixed_string("verbose"));
    detail::ParseContextWriter::set_value<bool>(*parent, h, true);

    ParseContext child;
    detail::ParseContextWriter::set_parent(child, parent);

    CHECK(child.has<fixed_string("verbose")>());
}

TEST_CASE("parent chain get<T,Key>() falls back to parent")
{
    auto parent = std::make_shared<ParseContext>();
    constexpr auto h = key_hash(fixed_string("port"));
    detail::ParseContextWriter::set_value<int>(*parent, h, 8080);

    ParseContext child;
    detail::ParseContextWriter::set_parent(child, parent);

    CHECK(child.get<int, fixed_string("port")>() == 8080);
}

TEST_CASE("parent chain get<T,Key>() prefers own value")
{
    auto parent = std::make_shared<ParseContext>();
    constexpr auto h_p = key_hash(fixed_string("port"));
    detail::ParseContextWriter::set_value<int>(*parent, h_p, 8080);

    ParseContext child;
    detail::ParseContextWriter::set_parent(child, parent);
    constexpr auto h_c = key_hash(fixed_string("port"));
    detail::ParseContextWriter::set_value<int>(child, h_c, 9090);

    CHECK(child.get<int, fixed_string("port")>() == 9090);
}

TEST_CASE("parent chain try_get falls back to parent")
{
    auto parent = std::make_shared<ParseContext>();
    constexpr auto h = key_hash(fixed_string("verbose"));
    detail::ParseContextWriter::set_value<bool>(*parent, h, true);

    ParseContext child;
    detail::ParseContextWriter::set_parent(child, parent);

    auto r = child.try_get<bool, fixed_string("verbose")>();
    CHECK(r.is_some());
    CHECK(r.unwrap() == true);
}

TEST_CASE("parent chain try_get returns None when neither has value")
{
    auto parent = std::make_shared<ParseContext>();
    ParseContext child;
    detail::ParseContextWriter::set_parent(child, parent);

    CHECK_FALSE(child.try_get<int, fixed_string("missing")>().is_some());
}

TEST_CASE("parent chain get_or falls back to parent")
{
    auto parent = std::make_shared<ParseContext>();
    constexpr auto h = key_hash(fixed_string("port"));
    detail::ParseContextWriter::set_value<int>(*parent, h, 8080);

    ParseContext child;
    detail::ParseContextWriter::set_parent(child, parent);

    CHECK(child.get_or<int, fixed_string("port")>(999) == 8080);
}

TEST_CASE("parent chain get_or returns fallback when neither has value")
{
    auto parent = std::make_shared<ParseContext>();
    ParseContext child;
    detail::ParseContextWriter::set_parent(child, parent);

    CHECK(child.get_or<int, fixed_string("port")>(999) == 999);
}

TEST_CASE("parent chain own absent value falls back to parent try_get")
{
    auto parent = std::make_shared<ParseContext>();
    constexpr auto h = key_hash(fixed_string("name"));
    detail::ParseContextWriter::set_value<std::string>(*parent, h, std::string("parent"));

    ParseContext child;
    detail::ParseContextWriter::set_parent(child, parent);
    // Child has its own "port" but not "name"
    constexpr auto h2 = key_hash(fixed_string("port"));
    detail::ParseContextWriter::set_value<int>(child, h2, 111);

    auto r = child.try_get<std::string, fixed_string("name")>();
    CHECK(r.is_some());
    CHECK(r.unwrap() == "parent");
}

TEST_CASE("parent chain deep nesting all levels visible")
{
    auto root = std::make_shared<ParseContext>();
    constexpr auto h1 = key_hash(fixed_string("level1"));
    detail::ParseContextWriter::set_value<int>(*root, h1, 1);

    auto mid = std::make_shared<ParseContext>();
    constexpr auto h2 = key_hash(fixed_string("level2"));
    detail::ParseContextWriter::set_value<int>(*mid, h2, 2);
    detail::ParseContextWriter::set_parent(*mid, root);

    ParseContext leaf;
    constexpr auto h3 = key_hash(fixed_string("level3"));
    detail::ParseContextWriter::set_value<int>(leaf, h3, 3);
    detail::ParseContextWriter::set_parent(leaf, mid);

    CHECK(leaf.get<int, fixed_string("level3")>() == 3);
    CHECK(leaf.get<int, fixed_string("level2")>() == 2);
    CHECK(leaf.get<int, fixed_string("level1")>() == 1);
}

TEST_CASE("extra args accumulate at the root of the parent chain")
{
    auto parent = std::make_shared<ParseContext>();
    detail::ParseContextWriter::add_extra_arg(*parent, "a");

    ParseContext child;
    detail::ParseContextWriter::set_parent(child, parent);
    detail::ParseContextWriter::add_extra_arg(child, "b");

    REQUIRE(child.extra_args().size() == 2);
    CHECK(child.extra_args()[0] == "a");
    CHECK(child.extra_args()[1] == "b");
    // Root-owner: the parent exposes the same collection.
    CHECK(parent->extra_args().size() == 2);
}

TEST_CASE("extra args on a standalone context are unchanged")
{
    ParseContext ctx;
    detail::ParseContextWriter::add_extra_arg(ctx, "x");
    REQUIRE(ctx.extra_args().size() == 1);
    CHECK(ctx.extra_args()[0] == "x");
}

// ── Derived presence (no separate presence set) ──

TEST_CASE("has_value derives presence from every scalar map")
{
    ParseContext ctx;
    constexpr auto hb = key_hash(fixed_string("b"));
    constexpr auto hi = key_hash(fixed_string("i"));
    constexpr auto hd = key_hash(fixed_string("d"));
    constexpr auto hs = key_hash(fixed_string("s"));
    constexpr auto hp = key_hash(fixed_string("p"));
    detail::ParseContextWriter::set_value<bool>(ctx, hb, true);
    detail::ParseContextWriter::set_value<int>(ctx, hi, 1);
    detail::ParseContextWriter::set_value<double>(ctx, hd, 1.5);
    detail::ParseContextWriter::set_value<std::string>(ctx, hs, std::string("s"));
    detail::ParseContextWriter::set_value<std::filesystem::path>(
        ctx, hp, std::filesystem::path("p"));

    CHECK(detail::ParseContextWriter::has_value(ctx, hb));
    CHECK(detail::ParseContextWriter::has_value(ctx, hi));
    CHECK(detail::ParseContextWriter::has_value(ctx, hd));
    CHECK(detail::ParseContextWriter::has_value(ctx, hs));
    CHECK(detail::ParseContextWriter::has_value(ctx, hp));
    CHECK(ctx.has<fixed_string("b")>());
    CHECK_FALSE(ctx.has<fixed_string("missing")>());
}

TEST_CASE("has_value derives presence from vector maps")
{
    ParseContext ctx;
    constexpr auto h = key_hash(fixed_string("tag"));
    detail::ParseContextWriter::append_value<std::string>(ctx, h, std::string("a"));
    detail::ParseContextWriter::append_value<std::string>(ctx, h, std::string("b"));

    CHECK(detail::ParseContextWriter::has_value(ctx, h));
    CHECK(ctx.has<fixed_string("tag")>());
    auto &all = ctx.get_all<std::string, fixed_string("tag")>();
    REQUIRE(all.size() == 2);
    CHECK(all[0] == "a");
    CHECK(all[1] == "b");
}

TEST_CASE("set_value overwrite keeps presence")
{
    ParseContext ctx;
    constexpr auto h = key_hash(fixed_string("port"));
    detail::ParseContextWriter::set_value<int>(ctx, h, 1);
    detail::ParseContextWriter::set_value<int>(ctx, h, 2);
    CHECK(detail::ParseContextWriter::has_value(ctx, h));
    CHECK(ctx.get<int, fixed_string("port")>() == 2);
}

// ── Deep parent chain: lookup and teardown must not recurse ──

TEST_CASE("parent chain lookup is iterative at deep nesting")
{
    constexpr size_t kDepth = 10000;
    constexpr auto h = key_hash(fixed_string("deep"));
    constexpr auto hv = key_hash(fixed_string("list"));

    auto root = std::make_shared<ParseContext>();
    detail::ParseContextWriter::set_value<int>(*root, h, 7);
    detail::ParseContextWriter::append_value<int>(*root, hv, 1);

    std::shared_ptr<ParseContext> leaf = root;
    for (size_t i = 0; i < kDepth; ++i)
    {
        auto next = std::make_shared<ParseContext>();
        detail::ParseContextWriter::set_parent(*next, leaf);
        leaf = std::move(next);
    }

    CHECK(leaf->get<int, fixed_string("deep")>() == 7);           // non-const scalar
    CHECK(leaf->try_get<int, fixed_string("deep")>().is_some());  // const scalar
    CHECK(leaf->get_or<int, fixed_string("deep")>(0) == 7);       // const scalar
    CHECK(leaf->has<fixed_string("deep")>());                     // has_in_chain
    CHECK_FALSE(leaf->has<fixed_string("missing")>());
    REQUIRE(leaf->get_all<int, fixed_string("list")>().size() == 1);  // vector
}

TEST_CASE("parent chain keeps nearest value wins at deep nesting")
{
    constexpr size_t kDepth = 10000;
    constexpr auto h = key_hash(fixed_string("winner"));

    auto root = std::make_shared<ParseContext>();
    detail::ParseContextWriter::set_value<int>(*root, h, 1);

    std::shared_ptr<ParseContext> leaf = root;
    for (size_t i = 0; i < kDepth; ++i)
    {
        auto next = std::make_shared<ParseContext>();
        detail::ParseContextWriter::set_parent(*next, leaf);
        leaf = std::move(next);
    }
    detail::ParseContextWriter::set_value<int>(*leaf, h, 2);

    CHECK(leaf->get<int, fixed_string("winner")>() == 2);
    CHECK(leaf->try_get<int, fixed_string("winner")>().unwrap() == 2);
    CHECK(leaf->get_or<int, fixed_string("winner")>(0) == 2);
}

TEST_CASE("deep parent chain destruction is iterative")
{
    constexpr size_t kDepth = 20000;
    auto root = std::make_shared<ParseContext>();
    std::shared_ptr<ParseContext> leaf = root;
    for (size_t i = 0; i < kDepth; ++i)
    {
        auto next = std::make_shared<ParseContext>();
        detail::ParseContextWriter::set_parent(*next, leaf);
        leaf = std::move(next);
    }
    CHECK(leaf.use_count() == 1);
    root.reset();  // the leaf now sole-owns the whole chain
    CHECK(leaf.use_count() == 1);
    leaf.reset();  // iterative teardown; no per-level stack frame
}

TEST_CASE("shared parent chain survives one owner's destruction")
{
    constexpr auto h = key_hash(fixed_string("shared"));
    auto ancestor = std::make_shared<ParseContext>();
    detail::ParseContextWriter::set_value<int>(*ancestor, h, 42);

    auto owner_a = std::make_shared<ParseContext>();
    detail::ParseContextWriter::set_parent(*owner_a, ancestor);
    auto owner_b = std::make_shared<ParseContext>();
    detail::ParseContextWriter::set_parent(*owner_b, ancestor);

    REQUIRE(ancestor.use_count() == 3);
    owner_a.reset();  // must stop at the parent still shared with owner_b
    CHECK(ancestor.use_count() == 2);
    CHECK(owner_b->get<int, fixed_string("shared")>() == 42);
    CHECK(ancestor->get<int, fixed_string("shared")>() == 42);

    owner_b.reset();
    CHECK(ancestor.use_count() == 1);
    CHECK(ancestor->get<int, fixed_string("shared")>() == 42);
}
