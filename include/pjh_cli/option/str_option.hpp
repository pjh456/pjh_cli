#ifndef INCLUDE_PJH_CLI_OPTION_STR_OPTION_HPP
#define INCLUDE_PJH_CLI_OPTION_STR_OPTION_HPP

#include <algorithm>
#include <pjh_result.hpp>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "../error.hpp"
#include "../option_def.hpp"
#include "../parse_context.hpp"
#include "pjh_cli/type.hpp"

namespace pjh::cli
{
    /// @brief String-valued option.
    ///
    /// Created by `OptionBuilder::str()`.  Overrides `parse_value()` and
    /// `apply_default()` to store values as `std::string` in `ParseContext`.
    /// Supports optional value-set validation via `.choices()`.
    class StrOption : public OptionDef
    {
    public:
        /// @brief Construct with no default value.
        StrOption() : m_default(pjh::result::Option<std::string>::None()) {}

        /// @brief Whether a default string value has been set.
        bool has_default() const noexcept override { return m_default.is_some(); }

        /// @brief Store the raw string, then validate against choices if set.
        CliResult<void> parse_value(
            ParseContext &ctx, std::string_view raw) const override
        {
            auto s = std::string(raw);
            if (!m_choices.empty() && std::ranges::find(m_choices, s) == m_choices.end())
            {
                return CliFailure{invalid_choice(m_long_name, raw, m_choices)};
            }
            return store_or_append(ctx, m_key_hash, std::move(s));
        }

        /// @brief Apply the default string value if @p ctx has none.
        CliResult<void> apply_default(ParseContext &ctx) const override
        {
            if (m_default.is_some() && !ctx.has_value(m_key_hash))
                return store_or_append(ctx, m_key_hash, m_default.unwrap());
            return CliResult<void>::Ok();
        }

        /// @brief Register a typed default value.
        /// @return *this for chaining.
        StrOption &default_value(std::string v)
        {
            m_default = decltype(m_default)::Some(std::move(v));
            return *this;
        }

        /// @brief Restrict values to an explicit set.
        /// @param vals Allowed values (e.g. {"json", "yaml"}).
        /// @return *this for chaining.
        StrOption &choices(std::vector<std::string> vals)
        {
            m_choices = std::move(vals);
            return *this;
        }

    private:
        pjh::result::Option<std::string> m_default;
        std::vector<std::string> m_choices;
    };

}  // namespace pjh::cli

#endif  // INCLUDE_PJH_CLI_OPTION_STR_OPTION_HPP
