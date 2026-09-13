#include <pjh_cli/console/ring_buffer_history.hpp>
#include <stdexcept>
#include <utility>

namespace pjh::cli
{
    using Option = pjh::result::Option<std::string>;

    RingBufferHistory::RingBufferHistory(size_t max_size) : m_storage(max_size)
    {
        if (max_size == 0)
            throw std::invalid_argument("RingBufferHistory: max_size must be > 0");
    }

    void RingBufferHistory::push(std::string line)
    {
        (void)m_storage.push(std::move(line));
    }

    auto RingBufferHistory::prev() -> Option { return m_storage.prev(); }

    auto RingBufferHistory::next() -> Option { return m_storage.next(); }

    void RingBufferHistory::reset_cursor() { m_storage.reset_cursor(); }

    void RingBufferHistory::clear() { m_storage.clear(); }

    size_t RingBufferHistory::size() const noexcept { return m_storage.size(); }

}  // namespace pjh::cli
