#include <pjh_cli/detail/env_snapshot.hpp>
#include <pjh_platform/env.hpp>
#include <utility>

namespace pjh::cli::detail
{
    EnvSnapshot::EnvSnapshot()
    {
        auto snap = pjh::platform::Env::snapshot();
        m_env.reserve(snap.size());
        for (auto &[k, v] : snap) m_env.emplace(std::move(k), std::move(v));
    }
}  // namespace pjh::cli::detail
