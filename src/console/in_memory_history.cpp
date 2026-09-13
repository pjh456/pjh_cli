#include <pjh_cli/console/in_memory_history.hpp>
#include <utility>

namespace pjh::cli
{
    using Option = pjh::result::Option<std::string>;

    void InMemoryHistory::push(std::string line)
    {
        (void)m_storage.push(std::move(line));
    }

    auto InMemoryHistory::prev() -> Option { return m_storage.prev(); }

    auto InMemoryHistory::next() -> Option { return m_storage.next(); }

    void InMemoryHistory::reset_cursor() { m_storage.reset_cursor(); }

    void InMemoryHistory::clear() { m_storage.clear(); }

    size_t InMemoryHistory::size() const noexcept { return m_storage.size(); }

}  // namespace pjh::cli
