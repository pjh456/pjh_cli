#ifndef INCLUDE_PJH_CLI_DETAIL_ENV_SNAPSHOT_HPP
#define INCLUDE_PJH_CLI_DETAIL_ENV_SNAPSHOT_HPP

#include <pjh_cli/detail/string_utils.hpp>
#include <string>
#include <string_view>
#include <unordered_map>

namespace pjh::cli::detail
{

    class EnvSnapshot
    {
        // Transparent key/equality: get() looks up with string_view directly,
        // so no temporary std::string key is built per lookup.
        std::unordered_map<
            std::string,
            std::string,
            transparent_string_hash,
            std::equal_to<void>>
            m_env;

    public:
        /// @brief Capture the process environment at construction.
        ///
        /// The platform read lives in src/env.cpp; this header stays free of
        /// platform includes so including it never pulls <windows.h> into a
        /// translation unit.
        EnvSnapshot();

        /// @brief Look up @p name in the captured environment.
        ///
        /// Heterogeneous lookup: no temporary key is built, so the lookup
        /// allocates nothing and cannot throw.
        ///
        /// @param name Environment variable name.
        /// @return Pointer to the value, or nullptr if absent.
        const std::string *get(std::string_view name) const noexcept
        {
            auto it = m_env.find(name);
            if (it == m_env.end())
                return nullptr;
            return &it->second;
        }
    };

}  // namespace pjh::cli::detail

#endif
