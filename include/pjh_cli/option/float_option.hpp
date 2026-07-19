#ifndef INCLUDE_PJH_CLI_OPTION_FLOAT_OPTION_HPP
#define INCLUDE_PJH_CLI_OPTION_FLOAT_OPTION_HPP

#include "../converter.hpp"
#include "../option_def.hpp"
#include "../parse_context.hpp"

namespace pjh::cli
{
    /// @brief Double-valued floating-point option.
    class FloatOption : public OptionDef
    {
    public:
        FloatOption() : m_default(pjh::result::Option<double>::None()) {}

        bool has_default() const noexcept override { return m_default.is_some(); }

        CliResult<void> parse_value(
            ParseContext &ctx, std::string_view raw) const override
        {
            auto r = Converter<double>::from_string(raw);
            if (r.is_err())
                return CliResult<void>::Err(std::move(r).unwrap_err());
            ctx.set_value<double>(m_key_hash, r.unwrap());
            return CliResult<void>::Ok();
        }

        CliResult<void> apply_default(ParseContext &ctx) const override
        {
            if (m_default.is_some() && !ctx.has_value(m_key_hash))
                ctx.set_value<double>(m_key_hash, m_default.unwrap());
            return CliResult<void>::Ok();
        }

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
