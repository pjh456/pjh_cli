#include <pjh_cli/console/in_memory_history.hpp>
#include <utility>

namespace pjh::cli
{
    using Option = pjh::result::Option<std::string>;

    void InMemoryHistory::push(std::string line)
    {
        if (line.empty())
            return;
        if (!m_lines.empty() && m_lines.back() == line)
            return;
        m_lines.push_back(std::move(line));
        m_cursor = m_lines.size();
    }

    auto InMemoryHistory::prev() -> Option
    {
        if (m_lines.empty() || m_cursor == 0)
            return Option::None();
        --m_cursor;
        return Option::Some(m_lines[m_cursor]);
    }

    auto InMemoryHistory::next() -> Option
    {
        if (m_lines.empty() || m_cursor >= m_lines.size() - 1)
        {
            m_cursor = m_lines.size();
            return Option::None();
        }
        ++m_cursor;
        return Option::Some(m_lines[m_cursor]);
    }

    void InMemoryHistory::reset_cursor()
    {
        m_cursor = m_lines.size();
    }

    void InMemoryHistory::clear()
    {
        m_lines.clear();
        m_cursor = 0;
    }

    size_t InMemoryHistory::size() const noexcept
    {
        return m_lines.size();
    }

}  // namespace pjh::cli
