#ifndef INCLUDE_PJH_CLI_CONSOLE_DETAIL_HISTORY_STORAGE_HPP
#define INCLUDE_PJH_CLI_CONSOLE_DETAIL_HISTORY_STORAGE_HPP

#include <cstddef>
#include <deque>
#include <pjh_result.hpp>
#include <string>
#include <utility>

namespace pjh::cli::detail
{
    /// @brief Outcome of HistoryStorage::push().
    enum class HistoryInsert
    {
        Ignored,           ///< Empty line or consecutive duplicate; nothing stored.
        Stored,            ///< Appended without eviction.
        StoredAndTrimmed,  ///< Appended and the oldest entry was evicted by the cap.
    };

    /// @brief Shared entry storage and navigation cursor for the IHistory
    ///        backends (in-memory, ring buffer, file).
    ///
    /// A @p max_entries of 0 means unlimited; a positive cap evicts the oldest
    /// entry on overflow.  Navigation follows the IHistory contract: the cursor
    /// starts "past end", prev() walks toward older entries and stops at the
    /// oldest, next() walks toward newer entries and parks "past end".
    ///
    /// @internal Console implementation detail; not part of the public API.
    class HistoryStorage
    {
    public:
        /// @param max_entries  Retained-entry cap; 0 means unlimited.
        explicit HistoryStorage(std::size_t max_entries = 0) noexcept :
            m_max_entries(max_entries)
        {
        }

        /// @brief Store @p line unless it is empty or a consecutive duplicate,
        ///        evicting the oldest entry when the cap is reached; resets the
        ///        cursor to "past end" only on a stored line.
        HistoryInsert push(std::string line)
        {
            if (line.empty())
                return HistoryInsert::Ignored;
            if (!m_lines.empty() && m_lines.back() == line)
                return HistoryInsert::Ignored;
            bool trimmed = false;
            if (m_max_entries != 0 && m_lines.size() == m_max_entries)
            {
                m_lines.pop_front();
                trimmed = true;
            }
            m_lines.push_back(std::move(line));
            m_cursor = m_lines.size();
            return trimmed ? HistoryInsert::StoredAndTrimmed : HistoryInsert::Stored;
        }

        /// @brief Move one step toward older entries; None at the oldest.
        auto prev() -> pjh::result::Option<std::string>
        {
            if (m_lines.empty() || m_cursor == 0)
                return pjh::result::Option<std::string>::None();
            --m_cursor;
            return pjh::result::Option<std::string>::Some(m_lines[m_cursor]);
        }

        /// @brief Move one step toward newer entries; None past the newest and
        ///        parks the cursor "past end".
        auto next() -> pjh::result::Option<std::string>
        {
            if (m_lines.empty() || m_cursor >= m_lines.size() - 1)
            {
                m_cursor = m_lines.size();
                return pjh::result::Option<std::string>::None();
            }
            ++m_cursor;
            return pjh::result::Option<std::string>::Some(m_lines[m_cursor]);
        }

        /// @brief Reset the cursor to "past end".
        void reset_cursor() noexcept { m_cursor = m_lines.size(); }

        /// @brief Drop every entry and reset the cursor.
        void clear() noexcept
        {
            m_lines.clear();
            m_cursor = 0;
        }

        /// @brief Number of stored entries.
        std::size_t size() const noexcept { return m_lines.size(); }

        /// @brief Retained-entry cap; 0 means unlimited.
        std::size_t max_entries() const noexcept { return m_max_entries; }

        /// @brief Retained entries, oldest first (used by persistence backends).
        const std::deque<std::string> &lines() const noexcept { return m_lines; }

    private:
        std::size_t m_max_entries = 0;
        std::deque<std::string> m_lines;
        std::size_t m_cursor = 0;
    };

}  // namespace pjh::cli::detail

#endif  // INCLUDE_PJH_CLI_CONSOLE_DETAIL_HISTORY_STORAGE_HPP
