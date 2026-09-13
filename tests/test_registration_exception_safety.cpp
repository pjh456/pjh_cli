#include <doctest/doctest.h>

#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <new>
#include <pjh_cli/app.hpp>
#include <pjh_cli/command/branch_command.hpp>
#include <pjh_cli/core/fixed_string.hpp>
#include <pjh_cli/option/option_def.hpp>
#include <string>

using namespace pjh::cli;

namespace
{
    // ── Allocation-failure injection (test-binary only) ──
    //
    // Replaces the scalar global operator new/delete.  Both arming modes are
    // process-local atomics that stay idle (fail_after < 0, counting == false)
    // outside the armed windows:
    //   * counting:  allocations are counted while measure_allocations() runs;
    //   * budget:    the first `fail_after` allocations succeed and the next
    //                one throws std::bad_alloc.
    // doctest_discover_tests runs each TEST_CASE in its own process, so the
    // state cannot leak between cases; while both atomics are idle the override
    // is a transparent malloc/free pass-through.  The bad_alloc object itself
    // is allocated by the runtime, not through operator new, so rethrow and
    // unwind are unaffected.
    std::atomic<bool> g_counting{false};
    std::atomic<long> g_alloc_count{0};
    std::atomic<long> g_fail_after{-1};

    template <typename F>
    long measure_allocations(F &&f)
    {
        g_alloc_count.store(0, std::memory_order_relaxed);
        g_counting.store(true, std::memory_order_relaxed);
        f();
        g_counting.store(false, std::memory_order_relaxed);
        return g_alloc_count.load(std::memory_order_relaxed);
    }
}  // namespace

void *operator new(std::size_t n)
{
    long budget = g_fail_after.load(std::memory_order_relaxed);
    if (budget >= 0)
    {
        if (budget == 0)
            throw std::bad_alloc{};
        g_fail_after.store(budget - 1, std::memory_order_relaxed);
    }
    if (g_counting.load(std::memory_order_relaxed))
        g_alloc_count.fetch_add(1, std::memory_order_relaxed);
    if (void *p = std::malloc(n == 0 ? 1 : n))
        return p;
    throw std::bad_alloc{};
}

void operator delete(void *p) noexcept { std::free(p); }

TEST_CASE("add_option leaves the command unchanged when indexing allocation fails")
{
    App app("test", "1.0", "OOM");
    app.option<fixed_string("warm")>("--warm", "Warm the deque block").boolean();

    auto opt = std::make_unique<OptionDef>();
    opt->set_long_name("zzz");
    opt->set_short_name('z');

    bool threw = false;
    // The option is already owned and the deque block is warm, so the first
    // allocation inside add_option() is the index insert: fail it.
    g_fail_after.store(0, std::memory_order_relaxed);
    try
    {
        app.add_option(std::move(opt));
    }
    catch (const std::bad_alloc &)
    {
        threw = true;
    }
    g_fail_after.store(-1, std::memory_order_relaxed);

    CHECK(threw);
    CHECK(app.options().size() == 1);                  // pre-warm survives
    CHECK(app.find_option_by_long("zzz") == nullptr);  // no dangling long entry
    CHECK(app.find_option_by_short('z') == nullptr);   // no dangling short entry
}

TEST_CASE("add_leaf leaves the branch unchanged when name-index allocation fails")
{
    App app("test", "1.0", "OOM");
    app.add_leaf("warm", "Warm the deque block");

    // Calibrate on a throwaway tree: count the allocations one successful
    // add_leaf() performs in this process (child construction + the
    // name-index node).  The node is the last one, so letting `total - 1`
    // allocations succeed fails exactly the index insert.  The warm child
    // pre-creates the deque block and the map buckets, so both the real and
    // the measured call follow the same allocation path.
    auto total = [&]
    {
        App cal("cal", "1.0", "Cal");
        cal.add_leaf("warm", "Warm the deque block");
        return measure_allocations([&] { cal.add_leaf("measure", "Measure"); });
    }();

    bool threw = false;
    g_fail_after.store(total - 1, std::memory_order_relaxed);
    try
    {
        app.add_leaf("zzz", "New child");
    }
    catch (const std::bad_alloc &)
    {
        threw = true;
    }
    g_fail_after.store(-1, std::memory_order_relaxed);

    CHECK(threw);
    CHECK(app.subcommands().size() == 1);
    CHECK(app.find_subcommand("zzz") == nullptr);
}

TEST_CASE("add_branch leaves the branch unchanged when name-index allocation fails")
{
    App app("test", "1.0", "OOM");
    app.add_branch("warm", "Warm the deque block");

    auto total = [&]
    {
        App cal("cal", "1.0", "Cal");
        cal.add_branch("warm", "Warm the deque block");
        return measure_allocations([&] { cal.add_branch("measure", "Measure"); });
    }();

    bool threw = false;
    g_fail_after.store(total - 1, std::memory_order_relaxed);
    try
    {
        app.add_branch("zzz", "New child");
    }
    catch (const std::bad_alloc &)
    {
        threw = true;
    }
    g_fail_after.store(-1, std::memory_order_relaxed);

    CHECK(threw);
    CHECK(app.subcommands().size() == 1);
    CHECK(app.find_subcommand("zzz") == nullptr);
}

TEST_CASE("alias() rolls back its append when parent indexing allocation fails")
{
    App app("test", "1.0", "OOM");
    auto &child = app.add_leaf("child", "Child");

    // Pre-grow the child's alias vector so the measured push_back cannot
    // allocate: MSVC grows 1.5x (five pushes leave capacity 6) while
    // libstdc++/libc++ grow 2x (capacity 8), so the sixth push is
    // allocation-free on all of them.  The short keys fit the string SSO and
    // the parent index already holds enough entries that inserting "zzz" does
    // not force a rehash, so the only allocation left inside alias() is the
    // parent index node insert -- the path exercised by add_leaf/add_branch.
    for (const char *a : {"a1", "a2", "a3", "a4", "a5"}) child.alias(a);
    REQUIRE(child.aliases().size() == 5);

    bool threw = false;
    g_fail_after.store(0, std::memory_order_relaxed);
    try
    {
        child.alias("zzz");
    }
    catch (const std::bad_alloc &)
    {
        threw = true;
    }
    g_fail_after.store(-1, std::memory_order_relaxed);

    CHECK(threw);
    CHECK(child.aliases().size() == 5);  // "zzz" rolled back, pre-grown aliases survive
    CHECK(app.find_subcommand("zzz") == nullptr);
    CHECK(app.find_subcommand("a1") == &child);  // pre-grown index entry survives
}

TEST_CASE("every owned option is reachable through both indexes")
{
    App app("test", "1.0", "Consistency");
    app.option<fixed_string("a")>("--alpha", 'a', "A").boolean();
    app.option<fixed_string("b")>("--beta", "B").integer();
    app.option<fixed_string("c")>("--gamma", 'g', "G").str();

    CHECK(app.options().size() == 3);
    for (const auto &p : app.options())
    {
        CHECK(app.find_option_by_long(p->long_name()) == p.get());
        if (p->short_name() != 0)
            CHECK(app.find_option_by_short(p->short_name()) == p.get());
    }
}

TEST_CASE("every owned child is reachable through the name index")
{
    App app("test", "1.0", "Consistency");
    auto &b = app.add_branch("branch", "Branch");
    auto &l = app.add_leaf("leaf", "Leaf");
    b.add_leaf("inner", "Inner");

    CHECK(app.subcommands().size() == 2);
    for (const auto &p : app.subcommands())
    {
        CHECK(app.find_subcommand(p->name()) == p.get());
        CHECK(p->parent() == &app);
    }
    CHECK(app.find_subcommand("branch") == &b);
    CHECK(app.find_subcommand("leaf") == &l);
}
