#ifndef INCLUDE_PJH_CLI_CONSOLE_HISTORY_HPP
#define INCLUDE_PJH_CLI_CONSOLE_HISTORY_HPP

#include <pjh_result.hpp>
#include <string>
#include <vector>

namespace pjh::cli
{

    /// @brief Interface for command history storage.
    ///
    /// Used by InteractiveConsole to persist previously entered command
    /// lines for up/down-arrow navigation.  The default implementation
    /// (InMemoryHistory) stores lines in a ring-like vector.
    class IHistory
    {
    public:
        virtual ~IHistory() = default;

        /// @brief Append a line to the end of history and reset the
        ///        navigation cursor to "at end" (next prev() call
        ///        returns the last entry).
        ///
        /// Empty lines are ignored.  Consecutive duplicate lines are
        /// collapsed to a single entry.
        virtual void push(std::string line) = 0;

        /// @brief Move the cursor one step backward (toward older entries)
        ///        and return the entry at the new position.
        ///
        /// If the cursor is already at the oldest entry, returns
        /// Option::None() and the cursor stays at the oldest entry.
        virtual auto prev() -> pjh::result::Option<std::string> = 0;

        /// @brief Move the cursor one step forward (toward newer entries)
        ///        and return the entry at the new position.
        ///
        /// If the cursor is already past the newest entry, returns
        /// Option::None() and the cursor stays at "past end".
        virtual auto next() -> pjh::result::Option<std::string> = 0;

        /// @brief Reset the navigation cursor to "past end" so that the
        ///        next prev() call returns the most recent entry.
        virtual void reset_cursor() = 0;

        /// @brief Clear all history entries and reset the cursor.
        virtual void clear() = 0;

        /// @brief Number of stored history entries.
        virtual size_t size() const noexcept = 0;
    };

    /// @brief Default in-memory history implementation backed by a vector.
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
        std::vector<std::string> m_lines;
        size_t m_cursor = 0;
    };

}  // namespace pjh::cli

#endif
