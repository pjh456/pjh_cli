#ifndef INCLUDE_PJH_CLI_CONSOLE_NOOP_HISTORY_HPP
#define INCLUDE_PJH_CLI_CONSOLE_NOOP_HISTORY_HPP

#include <pjh_cli/console/history.hpp>

namespace pjh::cli
{

    /// @brief No-op history implementation that discards all input.
    ///
    /// All pushes are silently ignored.  prev() and next() always return
    /// None.  Useful as a Null Object pattern replacement for nullptr,
    /// or for testing scenarios where history must be explicitly disabled
    /// but callers do not want null checks.
    class NoOpHistory : public IHistory
    {
    public:
        void push(std::string /*line*/) override {}

        auto prev() -> pjh::result::Option<std::string> override
        {
            return pjh::result::Option<std::string>::None();
        }

        auto next() -> pjh::result::Option<std::string> override
        {
            return pjh::result::Option<std::string>::None();
        }

        void reset_cursor() override {}
        void clear() override {}
        size_t size() const noexcept override { return 0; }
    };

}  // namespace pjh::cli

#endif
