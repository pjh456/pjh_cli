#ifndef INCLUDE_PJH_CLI_OPTION_FLOAT_OPTION_HPP
#define INCLUDE_PJH_CLI_OPTION_FLOAT_OPTION_HPP

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
    /// @brief Double-valued floating-point option.
    ///
    /// Created by `OptionBuilder::floating()`.  Overrides `parse_value()` and
    /// `apply_default()` to store values as `double` in `ParseContext`.
    /// Supports optional range validation via .min() / .max().
    class FloatOption : public OptionDef
    {
    public:
        FloatOption() :
            m_default(pjh::result::Option<double>::None()),
            m_min(pjh::result::Option<double>::None()),
            m_max(pjh::result::Option<double>::None())
        {
        }

        bool has_default() const noexcept override { return m_default.is_some(); }

        /// @brief Human-readable default value for help output.
        std::string default_value_str() const override
        {
            return m_default.is_some() ? std::to_string(m_default.unwrap()) : "";
        }

        CliResult<void> parse_value(
            ParseContext &ctx, std::string_view raw) const override
        {
            auto r = Converter<double>::from_string(raw);
            if (r.is_err())
                return CliResult<void>::Err(std::move(r).unwrap_err());
            double v = r.unwrap();
            if (m_min.is_some() && v < m_min.unwrap())
                return CliFailure{value_out_of_range(
                    m_long_name, raw, m_min.unwrap(),
                    m_max.is_some() ? m_max.unwrap() : v + 1)};
            if (m_max.is_some() && v > m_max.unwrap())
                return CliFailure{value_out_of_range(
                    m_long_name, raw, m_min.is_some() ? m_min.unwrap() : v - 1,
                    m_max.unwrap())};
            return store_or_append(ctx, m_key_hash, v);
        }

        CliResult<void> apply_default(ParseContext &ctx) const override
        {
            if (m_default.is_some() && !ctx.has_value(m_key_hash))
                return store_or_append(ctx, m_key_hash, m_default.unwrap());
            return CliResult<void>::Ok();
        }

        FloatOption &default_value(double v)
        {
            m_default = decltype(m_default)::Some(v);
            return *this;
        }

        FloatOption &min(double v)
        {
            m_min = decltype(m_min)::Some(v);
            return *this;
        }

        FloatOption &max(double v)
        {
            m_max = decltype(m_max)::Some(v);
            return *this;
        }

    private:
        pjh::result::Option<double> m_default;
        pjh::result::Option<double> m_min;
        pjh::result::Option<double> m_max;
    };

}  // namespace pjh::cli

#endif  // INCLUDE_PJH_CLI_OPTION_FLOAT_OPTION_HPP
