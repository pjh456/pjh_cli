#include <doctest/doctest.h>

#include <atomic>
#include <pjh_cli/app.hpp>
#include <pjh_cli/core/fixed_string.hpp>
#include <string>
#include <thread>
#include <vector>

using namespace pjh::cli;

TEST_CASE("concurrent parse on same App")
{
    App app("test", "1.0", "Concurrent");
    app.option<fixed_string("port")>("--port", 'p', "Port", 8080);

    constexpr int THREADS = 4;
    constexpr int ITERS = 50;
    std::atomic<int> ok_count{0};
    std::vector<std::thread> threads;

    for (int t = 0; t < THREADS; ++t)
    {
        threads.emplace_back([&app, &ok_count, t]()
        {
            for (int i = 0; i < ITERS; ++i)
            {
                std::vector<std::string> storage;
                storage.emplace_back("test");
                storage.emplace_back("--port");
                storage.emplace_back(std::to_string(8000 + i + t * ITERS));

                std::vector<char *> ptrs;
                for (auto &s : storage)
                    ptrs.push_back(s.data());

                auto r = app.parse(static_cast<int>(ptrs.size()), ptrs.data());
                if (r.is_ok())
                {
                    auto v = r.unwrap().get<int, fixed_string("port")>();
                    if (v == 8000 + i + t * ITERS)
                        ok_count.fetch_add(1);
                }
            }
        });
    }

    for (auto &th : threads)
        th.join();

    CHECK(ok_count == THREADS * ITERS);
}

TEST_CASE("concurrent parse different Apps")
{
    constexpr int THREADS = 4;
    constexpr int ITERS = 50;
    std::atomic<int> ok_count{0};
    std::vector<std::thread> threads;

    for (int t = 0; t < THREADS; ++t)
    {
        threads.emplace_back([&ok_count, t]()
        {
            App local("sub", "1.0", "Local");
            local.option<fixed_string("val")>("--val", 'v', "Value").integer();

            for (int i = 0; i < ITERS; ++i)
            {
                std::vector<std::string> storage;
                storage.emplace_back("sub");
                storage.emplace_back("--val");
                storage.emplace_back(std::to_string(i));

                std::vector<char *> ptrs;
                for (auto &s : storage)
                    ptrs.push_back(s.data());

                auto r = local.parse(static_cast<int>(ptrs.size()), ptrs.data());
                if (r.is_ok())
                {
                    auto v = r.unwrap().get<int, fixed_string("val")>();
                    if (v == i)
                        ok_count.fetch_add(1);
                }
            }
        });
    }

    for (auto &th : threads)
        th.join();

    CHECK(ok_count == THREADS * ITERS);
}
