#ifndef INCLUDE_PJH_CLI_OPTION_STR_OPTION_HPP
#define INCLUDE_PJH_CLI_OPTION_STR_OPTION_HPP

#include <algorithm>
#include <pjh_result.hpp>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "mixin/with_default.hpp"
#include "pjh_cli/error.hpp"
#include "pjh_cli/option_def.hpp"
#include "pjh_cli/type.hpp"

namespace pjh::cli
{

    /// @brief String-valued option with optional choices validation.
    class StrOption : public WithDefault<std::string, StrOption>
    {
        std::vector<std::string> m_choices;

    public:
        CliResult<void> parse_value(
            ParseContext &ctx, std::string_view raw) const override
        {
            auto s = std::string(raw);
            if (!m_choices.empty() &&
                std::ranges::find(m_choices, s) == m_choices.end())
            {
                return CliFailure{invalid_choice(m_long_name, raw, m_choices)};
            }
            return store_or_append(ctx, m_key_hash, std::move(s));
        }

        StrOption &choices(std::vector<std::string> vals)
        {
            m_choices = std::move(vals);
            return *this;
        }
    };

}  // namespace pjh::cli

#endif
