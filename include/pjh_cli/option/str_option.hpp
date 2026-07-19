#ifndef INCLUDE_PJH_CLI_OPTION_STR_OPTION_HPP
#define INCLUDE_PJH_CLI_OPTION_STR_OPTION_HPP

#include "../option_def.hpp"
#include "../parse_context.hpp"

namespace pjh::cli
{
    /// @brief String-valued option.  Soon: .choices(), .repeatable()
    class StrOption : public OptionDef
    {
    public:
        StrOption() : m_default(pjh::result::Option<std::string>::None()) {}

        bool has_default() const noexcept override { return m_default.is_some(); }

        CliResult<void> parse_value(
            ParseContext &ctx, std::string_view raw) const override
        {
            ctx.set_value<std::string>(m_key_hash, std::string(raw));
            return CliResult<void>::Ok();
        }

        CliResult<void> apply_default(ParseContext &ctx) const override
        {
            if (m_default.is_some() && !ctx.has_value(m_key_hash))
                ctx.set_value<std::string>(m_key_hash, m_default.unwrap());
            return CliResult<void>::Ok();
        }

        StrOption &default_value(std::string v)
        {
            m_default = decltype(m_default)::Some(std::move(v));
            return *this;
        }

    private:
        pjh::result::Option<std::string> m_default;
    };

}  // namespace pjh::cli

#endif  // INCLUDE_PJH_CLI_OPTION_STR_OPTION_HPP
