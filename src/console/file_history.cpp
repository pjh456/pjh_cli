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
        m_path(std::move(path)), m_max_entries(max_entries)
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

    bool FileHistory::insert(std::string line, bool &trimmed)
    {
        trimmed = false;
        if (line.empty())
            return false;
        if (!m_lines.empty() && m_lines.back() == line)
            return false;
        if (m_max_entries != 0 && m_lines.size() == m_max_entries)
        {
            m_lines.pop_front();
            trimmed = true;
        }
        m_lines.push_back(std::move(line));
        m_cursor = m_lines.size();
        return true;
    }

    void FileHistory::load()
    {
        bool changed = false;
        for (auto &line : read_lines(m_path))
        {
            bool trimmed = false;
            if (!insert(std::move(line), trimmed))
                changed = true;  // blank or duplicate line in the file
            if (trimmed)
                changed = true;  // cap dropped an old entry
        }
        if (changed)
            (void)save();  // normalize the file to the retained window
    }

    void FileHistory::push(std::string line)
    {
        bool trimmed = false;
        if (!insert(std::move(line), trimmed))
            return;
        if (trimmed || !append_line(m_path, m_lines.back()))
            (void)save();  // rewrite on trim, or repair a failed append
    }

    bool FileHistory::save() const
    {
        try
        {
            return write_lines(m_path, m_lines);
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
        m_lines.clear();
        m_cursor = 0;
        (void)truncate_file(m_path);
    }

    auto FileHistory::prev() -> Option
    {
        if (m_lines.empty() || m_cursor == 0)
            return Option::None();
        --m_cursor;
        return Option::Some(m_lines[m_cursor]);
    }

    auto FileHistory::next() -> Option
    {
        if (m_lines.empty() || m_cursor >= m_lines.size() - 1)
        {
            m_cursor = m_lines.size();
            return Option::None();
        }
        ++m_cursor;
        return Option::Some(m_lines[m_cursor]);
    }

    void FileHistory::reset_cursor() { m_cursor = m_lines.size(); }

    size_t FileHistory::size() const noexcept { return m_lines.size(); }

}  // namespace pjh::cli
