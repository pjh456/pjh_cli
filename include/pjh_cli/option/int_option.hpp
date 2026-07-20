#ifndef INCLUDE_PJH_CLI_OPTION_INT_OPTION_HPP
#define INCLUDE_PJH_CLI_OPTION_INT_OPTION_HPP

#include <pjh_result.hpp>
#include <string_view>
#include <utility>

#include "../converter.hpp"
#include "../error.hpp"
#include "../option_def.hpp"
#include "../parse_context.hpp"
#include "pjh_cli/type.hpp"

namespace pjh::cli
{
    /// @brief Integer-valued option.
    ///
    /// Created by `OptionBuilder::integer()`.  Overrides `parse_value()` and
    /// `apply_default()` to store values as `int` in `ParseContext`.
    /// Supports optional range validation via .min() / .max().
    class IntOption : public OptionDef
    {
    public:
        /// @brief Construct with no default, no min/max bounds.
        IntOption() :
            m_default(pjh::result::Option<int>::None()),
            m_min(pjh::result::Option<int>::None()),
            m_max(pjh::result::Option<int>::None())
        {
        }

        /// @brief Whether a default int value has been set.
        bool has_default() const noexcept override { return m_default.is_some(); }

        /// @brief Parse the raw string as an int and store in @p ctx.
        CliResult<void> parse_value(
            ParseContext &ctx, std::string_view raw) const override
        {
            auto r = Converter<int>::from_string(raw);
            if (r.is_err())
                return CliResult<void>::Err(std::move(r).unwrap_err());
            int v = r.unwrap();
            if (m_min.is_some() && v < m_min.unwrap())
                return CliFailure{value_out_of_range(
                    m_long_name, raw, m_min.unwrap(),
                    m_max.is_some() ? m_max.unwrap() : 2147483647)};
            if (m_max.is_some() && v > m_max.unwrap())
                return CliFailure{value_out_of_range(
                    m_long_name, raw, m_min.is_some() ? m_min.unwrap() : -2147483648,
                    m_max.unwrap())};
            return store_or_append(ctx, m_key_hash, v);
        }

        /// @brief Apply the default int value if @p ctx has none.
        CliResult<void> apply_default(ParseContext &ctx) const override
        {
            if (m_default.is_some() && !ctx.has_value(m_key_hash))
                return store_or_append(ctx, m_key_hash, m_default.unwrap());
            return CliResult<void>::Ok();
        }

        /// @brief Register a typed default value.
        /// @return *this for chaining.
        IntOption &default_value(int v)
        {
            m_default = decltype(m_default)::Some(v);
            return *this;
        }

        /// @brief Set the minimum allowed value (inclusive).
        /// @return *this for chaining.
        IntOption &min(int v)
        {
            m_min = decltype(m_min)::Some(v);
            return *this;
        }

        /// @brief Set the maximum allowed value (inclusive).
        /// @return *this for chaining.
        IntOption &max(int v)
        {
            m_max = decltype(m_max)::Some(v);
            return *this;
        }

    private:
        pjh::result::Option<int> m_default;
        pjh::result::Option<int> m_min;
        pjh::result::Option<int> m_max;
    };

}  // namespace pjh::cli

#endif  // INCLUDE_PJH_CLI_OPTION_INT_OPTION_HPP
