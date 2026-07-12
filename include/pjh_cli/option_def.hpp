#ifndef INCLUDE_PJH_CLI_OPTION_DEF_HPP
#define INCLUDE_PJH_CLI_OPTION_DEF_HPP

#include "type.hpp"

#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace pjh::cli
{
    class ParseContext;

    /// @brief Definition of a named option (flag or valued).
    ///
    /// Identified by compile-time fixed_string key. Bool options are flags
    /// (no value consumed); all other types consume the next token as value.
    struct OptionDef
    {
        std::string m_long_name;   ///< Long name (without `--`), e.g. "port"
        char m_short_name{};       ///< Short name, e.g. 'p'  (0 = none)
        std::string m_description; ///< Help text
        bool m_has_value{};        ///< true = consumes next token
        bool m_required{};         ///< Fail parse if not provided on command line
        size_t m_key_hash{};       ///< Runtime hash (== compile-time key_hash(Key))

        /// @brief Type-erased converter: parse string and store in context.
        std::function<
            ParseResult<void>(
                ParseContext &,
                std::string_view)>
            m_apply;

        /// @brief Type-erased default-setter: pre-fill context with default value.
        std::function<
            void(ParseContext &)>
            m_apply_default;

        /// @brief Optional completer callback for tab completion.
        std::function<
            std::vector<std::string>()>
            m_completer;

        /// @brief Mark this option as required.
        OptionDef &
        required(bool r = true)
        {
            m_required = r;
            return *this;
        }

        /// @brief Supply a completer for tab-completing this option's values.
        OptionDef &
        completer(
            std::function<
                std::vector<std::string>()>
                fn)
        {
            m_completer = std::move(fn);
            return *this;
        }
    };

} // namespace pjh::cli

#endif // INCLUDE_PJH_CLI_OPTION_DEF_HPP
