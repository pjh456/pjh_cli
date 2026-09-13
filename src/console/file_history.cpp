#include <deque>
#include <fstream>
#include <iterator>
#include <new>
#include <pjh_cli/console/file_history.hpp>
#include <pjh_platform/fs.hpp>
#include <utility>
#include <vector>

namespace
{

    /// @brief Read every line, stripping a single trailing '\r'.
    std::vector<std::string> read_lines(const std::filesystem::path &path)
    {
        std::vector<std::string> lines;
        std::ifstream in(path, std::ios::binary);
        if (!in)
            return lines;
        std::string line;
        while (std::getline(in, line))
        {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            lines.push_back(std::move(line));
        }
        return lines;
    }

    /// @brief Append one line + '\n'; false on open/write failure.
    bool append_line(const std::filesystem::path &path, const std::string &line)
    {
        std::string payload = line;
        payload.push_back('\n');
        return pjh::platform::Fs::append(path, payload).is_ok();
    }

    /// @brief Truncate + rewrite every line + '\n'; false on failure.
    bool write_lines(
        const std::filesystem::path &path, const std::deque<std::string> &lines)
    {
        std::ofstream out(path, std::ios::trunc | std::ios::binary);
        if (!out)
            return false;
        for (const auto &line : lines) out << line << '\n';
        out.flush();
        return out.good();
    }

    /// @brief Truncate to an empty file; false on failure.
    bool truncate_file(const std::filesystem::path &path)
    {
        std::ofstream out(path, std::ios::trunc | std::ios::binary);
        if (!out)
            return false;
        out.flush();
        return out.good();
    }

}  // namespace

namespace pjh::cli
{
    using Option = pjh::result::Option<std::string>;

    FileHistory::FileHistory(std::filesystem::path path, std::size_t max_entries) :
        m_path(std::move(path)), m_storage(max_entries)
    {
        load();
    }

    FileHistory::~FileHistory()
    {
        try
        {
            (void)save();
        }
        catch (...)
        {
            // Best-effort flush: a noexcept destructor must never terminate (e.g. OOM).
        }
    }

    void FileHistory::load()
    {
        bool changed = false;
        for (auto &line : read_lines(m_path))
        {
            // Ignored (blank/duplicate) or trimmed entries mean the file must be
            // normalized to the retained window.
            if (m_storage.push(std::move(line)) != detail::HistoryInsert::Stored)
                changed = true;
        }
        if (changed)
            (void)save();  // normalize the file to the retained window
    }

    void FileHistory::push(std::string line)
    {
        const auto result = m_storage.push(std::move(line));
        if (result == detail::HistoryInsert::Ignored)
            return;
        if (result == detail::HistoryInsert::StoredAndTrimmed ||
            !append_line(m_path, m_storage.lines().back()))
            (void)save();  // rewrite on trim, or repair a failed append
    }

    bool FileHistory::save() const
    {
        try
        {
            return write_lines(m_path, m_storage.lines());
        }
        catch (const std::bad_alloc &)
        {
            throw;  // OOM is not an I/O failure; the documented contract allows it.
        }
        catch (...)
        {
            return false;  // I/O failure: silent, non-fatal.
        }
    }

    void FileHistory::clear()
    {
        m_storage.clear();
        (void)truncate_file(m_path);
    }

    auto FileHistory::prev() -> Option { return m_storage.prev(); }

    auto FileHistory::next() -> Option { return m_storage.next(); }

    void FileHistory::reset_cursor() { m_storage.reset_cursor(); }

    size_t FileHistory::size() const noexcept { return m_storage.size(); }

}  // namespace pjh::cli
