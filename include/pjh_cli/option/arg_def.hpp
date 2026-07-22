#ifndef INCLUDE_PJH_CLI_ARG_DEF_HPP
#define INCLUDE_PJH_CLI_ARG_DEF_HPP

#include <cstddef>
#include <pjh_cli/core/type.hpp>
#include <string>

namespace pjh::cli
{
    /// @brief Definition of a single positional argument.
    ///
    /// Indexed by compile-time size_t.  The parser fills arguments in
    /// index order (0, 1, 2, …).  Created via LeafCommand::arg<T, Index>().
    struct ArgDef
    {
        std::string m_name;  ///< Display name for help / error messages (e.g. "src").
        std::string m_description;  ///< Help text description.
        bool m_required{};          ///< Fail parse if not provided.
        size_t m_key_hash{};        ///< Runtime hash (== compile-time key_hash(index)).
        ValueTag m_value_tag{};     ///< Type tag for runtime dispatch.

        /// @brief Mark this argument as required.
        /// @param r Pass false to make optional again.
        /// @return *this for chaining.
        ArgDef &required(bool r = true)
        {
            m_required = r;
            return *this;
        }
    };

}  // namespace pjh::cli

#endif  // INCLUDE_PJH_CLI_ARG_DEF_HPP
