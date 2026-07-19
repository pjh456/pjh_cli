#ifndef INCLUDE_PJH_CLI_OPTION_BOOL_OPTION_HPP
#define INCLUDE_PJH_CLI_OPTION_BOOL_OPTION_HPP

#include "../option_def.hpp"
#include "../parse_context.hpp"

namespace pjh::cli
{
    /// @brief Boolean flag option.
    ///
    /// Created by `OptionBuilder::boolean()`.  Flags do not consume a value
    /// token — the parser sets `true` directly when the flag is present.
    /// Overrides `apply_default()` to support a default bool value.
    /// Future chain methods: .negatable()
    class BoolOption : public OptionDef
    {
    public:
        /// @brief Construct with no default value.
        BoolOption() : m_default(pjh::result::Option<bool>::None()) {}

        /// @brief Whether a default bool value has been set.
        bool has_default() const noexcept override { return m_default.is_some(); }

        /// @brief Apply the default bool value if @p ctx has none.
        CliResult<void> apply_default(ParseContext &ctx) const override
        {
            if (m_default.is_some() && !ctx.has_value(m_key_hash))
                ctx.set_value<bool>(m_key_hash, m_default.unwrap());
            return CliResult<void>::Ok();
        }

        /// @brief Register a typed default value.
        /// @param v Default bool value.
        /// @return *this for chaining.
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
