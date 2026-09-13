#ifndef INCLUDE_PJH_CLI_CONSOLE_IN_MEMORY_HISTORY_HPP
#define INCLUDE_PJH_CLI_CONSOLE_IN_MEMORY_HISTORY_HPP

#include <cstddef>
#include <pjh_cli/console/detail/history_storage.hpp>
#include <pjh_cli/console/history.hpp>
#include <string>

namespace pjh::cli
{

    /// @brief Default in-memory history implementation backed by the shared
    ///        detail::HistoryStorage.
    ///
    /// Stores lines in insertion order.  Consecutive duplicate lines are
    /// automatically deduplicated.  The navigation cursor starts at
    /// "past end" and moves with prev() / next() calls.
    ///
    /// Thread safety: not thread-safe (same as InteractiveConsole).
    class InMemoryHistory : public IHistory
    {
    public:
        void push(std::string line) override;
        auto prev() -> pjh::result::Option<std::string> override;
        auto next() -> pjh::result::Option<std::string> override;
        void reset_cursor() override;
        void clear() override;
        size_t size() const noexcept override;

    private:
        detail::HistoryStorage m_storage;
    };

}  // namespace pjh::cli

#endif
