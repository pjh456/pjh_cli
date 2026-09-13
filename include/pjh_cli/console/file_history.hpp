#ifndef INCLUDE_PJH_CLI_CONSOLE_FILE_HISTORY_HPP
#define INCLUDE_PJH_CLI_CONSOLE_FILE_HISTORY_HPP

#include <cstddef>
#include <deque>
#include <filesystem>
#include <pjh_cli/console/history.hpp>
#include <string>

namespace pjh::cli
{

    /// @brief File-backed history that survives process restarts.
    ///
    /// Loads entries from @p path on construction (one entry per line),
    /// keeps them in memory for navigation, appends each newly stored line
    /// to the file immediately, and rewrites the file when the optional cap
    /// evicts an entry or when save() is called.  A missing or unreadable
    /// file starts an empty history; write failures are non-fatal (the entry
    /// stays in memory and save() reports false).
    ///
    /// Format: UTF-8 text, one entry per line, no escaping (entries are
    /// newline-free by construction).  A single trailing '\r' is stripped on
    /// load and blank lines are skipped.  Consecutive duplicate lines are
    /// collapsed (same as the other backends).
    ///
    /// @note Not thread-safe (same as InteractiveConsole).
    class FileHistory : public IHistory
    {
    public:
        /// @param path         Backing file.  Created on first write; parent
        ///                     directories are not created.  No '~' expansion.
        /// @param max_entries  Maximum entries to retain; 0 means unlimited.
        ///                     When exceeded, the oldest entry is dropped from
        ///                     memory and the file is rewritten.
        explicit FileHistory(std::filesystem::path path, std::size_t max_entries = 0);

        /// @note Best-effort save(); never throws (allocation failures are swallowed).
        ~FileHistory() override;

        /// @brief Rewrite the backing file from the retained entries.
        /// @return true when the file was written; false on any I/O error.
        /// @note Never throws for I/O errors; allocation failures may propagate.
        bool save() const;

        void push(std::string line) override;
        auto prev() -> pjh::result::Option<std::string> override;
        auto next() -> pjh::result::Option<std::string> override;
        void reset_cursor() override;
        void clear() override;
        size_t size() const noexcept override;

        /// @brief Backing file path.
        const std::filesystem::path &path() const noexcept { return m_path; }

        /// @brief Retained-entry cap; 0 means unlimited.
        size_t max_entries() const noexcept { return m_max_entries; }

    private:
        /// @brief Store @p line in memory, applying empty/duplicate/cap rules.
        /// @return true if the line was stored (not empty/duplicate).
        /// @param[out] trimmed  True when the cap evicted the oldest entry.
        bool insert(std::string line, bool &trimmed);

        /// @brief Read the file and seed memory (normalizing if needed).
        void load();

        std::filesystem::path m_path;
        size_t m_max_entries = 0;
        std::deque<std::string> m_lines;
        size_t m_cursor = 0;
    };

}  // namespace pjh::cli

#endif  // INCLUDE_PJH_CLI_CONSOLE_FILE_HISTORY_HPP
