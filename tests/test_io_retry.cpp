#include <doctest/doctest.h>

#include <cerrno>
#include <pjh_cli/detail/io_retry.hpp>

using namespace pjh::cli;

TEST_CASE("retry_on_eintr returns the first success")
{
    int calls = 0;
    const int result = detail::retry_on_eintr(
        [&calls]
        {
            ++calls;
            return 42;
        });
    CHECK(result == 42);
    CHECK(calls == 1);
}

TEST_CASE("retry_on_eintr retries EINTR until success")
{
    int calls = 0;
    const int result = detail::retry_on_eintr(
        [&calls]
        {
            ++calls;
            if (calls < 3)
            {
                errno = EINTR;
                return -1;
            }
            return 7;
        });
    CHECK(result == 7);
    CHECK(calls == 3);
}

TEST_CASE("retry_on_eintr stops on a non-EINTR error")
{
    errno = 0;
    const int result = detail::retry_on_eintr(
        []
        {
            errno = EIO;
            return -1;
        });
    CHECK(result == -1);
    CHECK(errno == EIO);
}

TEST_CASE("retry_on_eintr returns zero without retrying")
{
    int calls = 0;
    const int result = detail::retry_on_eintr(
        [&calls]
        {
            ++calls;
            return 0;
        });
    CHECK(result == 0);
    CHECK(calls == 1);
}
