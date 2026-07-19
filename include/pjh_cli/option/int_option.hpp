#ifndef INCLUDE_PJH_CLI_OPTION_INT_OPTION_HPP
#define INCLUDE_PJH_CLI_OPTION_INT_OPTION_HPP

#include "../converter.hpp"
#include "../option_def.hpp"
#include "../parse_context.hpp"

namespace pjh::cli
{
    /// @brief Integer-valued option.  Soon: .min(), .max(), .count()
    class IntOption : public OptionDef
    {
    public:
        IntOption() : m_default(pjh::result::Option<int>::None()) {}

        bool has_default() const noexcept override { return m_default.is_some(); }

        CliResult<void> parse_value(
            ParseContext &ctx, std::string_view raw) const override
        {
            auto r = Converter<int>::from_string(raw);
            if (r.is_err())
                return CliResult<void>::Err(std::move(r).unwrap_err());
            ctx.set_value<int>(m_key_hash, r.unwrap());
            return CliResult<void>::Ok();
        }

        CliResult<void> apply_default(ParseContext &ctx) const override
        {
            if (m_default.is_some() && !ctx.has_value(m_key_hash))
                ctx.set_value<int>(m_key_hash, m_default.unwrap());
            return CliResult<void>::Ok();
        }

        IntOption &default_value(int v)
        {
            m_default = decltype(m_default)::Some(v);
            return *this;
        }

    private:
        pjh::result::Option<int> m_default;
    };

}  // namespace pjh::cli

#endif  // INCLUDE_PJH_CLI_OPTION_INT_OPTION_HPP
