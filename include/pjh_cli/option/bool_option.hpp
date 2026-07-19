#ifndef INCLUDE_PJH_CLI_OPTION_BOOL_OPTION_HPP
#define INCLUDE_PJH_CLI_OPTION_BOOL_OPTION_HPP

#include "../option_def.hpp"
#include "../parse_context.hpp"

namespace pjh::cli
{
    /// @brief Boolean flag option.  Soon: .negatable()
    class BoolOption : public OptionDef
    {
    public:
        BoolOption() : m_default(pjh::result::Option<bool>::None()) {}

        bool has_default() const noexcept override { return m_default.is_some(); }

        CliResult<void> apply_default(ParseContext &ctx) const override
        {
            if (m_default.is_some() && !ctx.has_value(m_key_hash))
                ctx.set_value<bool>(m_key_hash, m_default.unwrap());
            return CliResult<void>::Ok();
        }

        BoolOption &default_value(bool v)
        {
            m_default = decltype(m_default)::Some(v);
            return *this;
        }

    private:
        pjh::result::Option<bool> m_default;
    };

}  // namespace pjh::cli

#endif  // INCLUDE_PJH_CLI_OPTION_BOOL_OPTION_HPP
