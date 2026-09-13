#ifndef INCLUDE_PJH_CLI_CONSOLE_RING_BUFFER_HISTORY_HPP
#define INCLUDE_PJH_CLI_CONSOLE_RING_BUFFER_HISTORY_HPP

#include <pjh_cli/console/detail/history_storage.hpp>
#include <pjh_cli/console/history.hpp>
#include <string>

namespace pjh::cli
{

    /// @brief Fixed-capacity history backed by a deque.
    ///
    /// When the number of entries exceeds @p max_size, the oldest entry
    /// is automatically discarded before the new one is inserted.  Useful
    /// for long-running REPL sessions to bound memory usage.
    ///
    /// Consecutive duplicate lines are collapsed (same as InMemoryHistory).
    class RingBufferHistory : public IHistory
    {
    public:
        /// @param max_size  Maximum number of entries to retain.
        /// @throws std::invalid_argument if max_size == 0.
        explicit RingBufferHistory(size_t max_size);

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
