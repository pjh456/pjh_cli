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
    /// Supports optional negation via `.negatable()`: `--no-verbose` sets
    /// the flag to `false`.
    class BoolOption : public OptionDef
    {
    public:
        /// @brief Construct with no default value.
        BoolOption() : m_default(pjh::result::Option<bool>::None()) {}

        /// @brief Whether a default bool value has been set.
        bool has_default() const noexcept override { return m_default.is_some(); }

        /// @brief Whether --no-xxx negation is enabled.
        bool is_negatable() const noexcept override { return m_negatable; }

        /// @brief Apply the default bool value if @p ctx has none.
        CliResult<void> apply_default(ParseContext &ctx) const override
        {
            if (m_default.is_some() && !ctx.has_value(m_key_hash))
                ctx.set_value<bool>(m_key_hash, m_default.unwrap());
            return CliResult<void>::Ok();
        }

        /// @brief Register a typed default value.
        /// @return *this for chaining.
        BoolOption &default_value(bool v)
        {
            m_default = decltype(m_default)::Some(v);
            return *this;
        }

        /// @brief Enable `--no-xxx` negation.  When `--no-verbose` is given,
        ///        the stored value is set to `false` instead of `true`.
        /// @return *this for chaining.
        BoolOption &negatable()
        {
            m_negatable = true;
            return *this;
        }

    private:
        pjh::result::Option<bool> m_default;
        bool m_negatable{};
    };

}  // namespace pjh::cli

#endif  // INCLUDE_PJH_CLI_OPTION_BOOL_OPTION_HPP
