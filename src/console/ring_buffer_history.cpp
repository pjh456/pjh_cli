#include <pjh_cli/console/ring_buffer_history.hpp>
#include <stdexcept>
#include <utility>

namespace pjh::cli
{
    using Option = pjh::result::Option<std::string>;

    RingBufferHistory::RingBufferHistory(size_t max_size) : m_max_size(max_size)
    {
        if (max_size == 0)
            throw std::invalid_argument("RingBufferHistory: max_size must be > 0");
    }

    void RingBufferHistory::push(std::string line)
    {
        if (line.empty())
            return;
        if (!m_lines.empty() && m_lines.back() == line)
            return;
        if (m_lines.size() == m_max_size)
            m_lines.pop_front();
        m_lines.push_back(std::move(line));
        m_cursor = m_lines.size();
    }

    auto RingBufferHistory::prev() -> Option
    {
        if (m_lines.empty() || m_cursor == 0)
            return Option::None();
        --m_cursor;
        return Option::Some(m_lines[m_cursor]);
    }

    auto RingBufferHistory::next() -> Option
    {
        if (m_lines.empty() || m_cursor >= m_lines.size() - 1)
        {
            m_cursor = m_lines.size();
            return Option::None();
        }
        ++m_cursor;
        return Option::Some(m_lines[m_cursor]);
    }

    void RingBufferHistory::reset_cursor() { m_cursor = m_lines.size(); }

    void RingBufferHistory::clear()
    {
        m_lines.clear();
        m_cursor = 0;
    }

    size_t RingBufferHistory::size() const noexcept { return m_lines.size(); }

}  // namespace pjh::cli
