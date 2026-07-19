#ifndef INCLUDE_PJH_CLI_OPTION_FLOAT_OPTION_HPP
#define INCLUDE_PJH_CLI_OPTION_FLOAT_OPTION_HPP

#include "../converter.hpp"
#include "../option_def.hpp"
#include "../parse_context.hpp"

namespace pjh::cli
{
    /// @brief Double-valued floating-point option.
    ///
    /// Created by `OptionBuilder::floating()`.  Overrides `parse_value()` and
    /// `apply_default()` to store values as `double` in `ParseContext`.
    class FloatOption : public OptionDef
    {
    public:
        /// @brief Construct with no default value.
        FloatOption() : m_default(pjh::result::Option<double>::None()) {}

        /// @brief Whether a default double value has been set.
        bool has_default() const noexcept override { return m_default.is_some(); }

        /// @brief Parse the raw string as a double and store in @p ctx.
        /// @param ctx Parse context.
        /// @param raw CLI token value to parse.
        /// @return Ok on success, or a CliError on invalid number format.
        CliResult<void> parse_value(
            ParseContext &ctx, std::string_view raw) const override
        {
            auto r = Converter<double>::from_string(raw);
            if (r.is_err())
                return CliResult<void>::Err(std::move(r).unwrap_err());
            ctx.set_value<double>(m_key_hash, r.unwrap());
            return CliResult<void>::Ok();
        }

        /// @brief Apply the default double value if @p ctx has none.
        CliResult<void> apply_default(ParseContext &ctx) const override
        {
            if (m_default.is_some() && !ctx.has_value(m_key_hash))
                ctx.set_value<double>(m_key_hash, m_default.unwrap());
            return CliResult<void>::Ok();
        }

        /// @brief Register a typed default value.
        /// @param v Default double value.
        /// @return *this for chaining.
        FloatOption &default_value(double v)
        {
            m_default = decltype(m_default)::Some(v);
            return *this;
        }

    private:
        pjh::result::Option<double> m_default;
    };

}  // namespace pjh::cli

#endif  // INCLUDE_PJH_CLI_OPTION_FLOAT_OPTION_HPP
