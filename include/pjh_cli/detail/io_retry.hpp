#ifndef INCLUDE_PJH_CLI_DETAIL_IO_RETRY_HPP
#define INCLUDE_PJH_CLI_DETAIL_IO_RETRY_HPP

#include <cerrno>

namespace pjh::cli::detail
{
    /// @brief Invoke @p op, retrying while it fails with @c EINTR.
    ///
    /// POSIX syscalls such as @c read and @c poll return -1 with
    /// @c errno == EINTR when interrupted by a signal delivered to a handler
    /// installed without @c SA_RESTART.  Such an interruption is not
    /// end-of-input and not a timeout, so the operation must be retried.
    ///
    /// @tparam Op  Nullary callable returning a signed integral value; a
    ///             negative result is treated as an error and inspected via
    ///             @c errno.
    /// @param op   Operation to invoke (e.g. a lambda wrapping @c ::read).
    /// @return The first result of @p op that is non-negative or has
    ///         @c errno != EINTR.
    /// @throws Any exception thrown by @p op.
    template <typename Op>
    auto retry_on_eintr(Op &&op) -> decltype(op())
    {
        for (;;)
        {
            auto result = op();
            if (result >= 0 || errno != EINTR)
                return result;
        }
    }
}  // namespace pjh::cli::detail

#endif  // INCLUDE_PJH_CLI_DETAIL_IO_RETRY_HPP
