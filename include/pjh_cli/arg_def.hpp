#ifndef INCLUDE_PJH_CLI_ARG_DEF_HPP
#define INCLUDE_PJH_CLI_ARG_DEF_HPP

#include <string>
#include <string_view>

#include "type.hpp"

namespace pjh::cli
{
    /// @brief Definition of a single positional argument.
    ///
    /// Indexed by compile-time size_t. The parser fills arguments in index order.
    struct ArgDef
    {
        std::string m_name;         ///< Display name for help / error messages
        std::string m_description;  ///< Help text
        size_t m_index{};           ///< Positional index
        bool m_required{};          ///< Fail parse if not provided
        size_t m_key_hash{};        ///< Runtime hash (== compile-time key_hash(index))
        ValueTag m_value_tag{};     ///< Type tag for runtime dispatch

        /// @brief Mark this argument as required.
        ArgDef &required(bool r = true)
        {
            m_required = r;
            return *this;
        }
    };

}  // namespace pjh::cli

#endif  // INCLUDE_PJH_CLI_ARG_DEF_HPP
