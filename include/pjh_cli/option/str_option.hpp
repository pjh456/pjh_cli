#ifndef INCLUDE_PJH_CLI_OPTION_STR_OPTION_HPP
#define INCLUDE_PJH_CLI_OPTION_STR_OPTION_HPP

#include "../option_def.hpp"
#include "../parse_context.hpp"

namespace pjh::cli
{
    /// @brief String-valued option.
    ///
    /// Created by `OptionBuilder::str()`.  Overrides `parse_value()` and
    /// `apply_default()` to store values as `std::string` in `ParseContext`.
    /// Future chain methods: .choices(), .repeatable()
    class StrOption : public OptionDef
    {
    public:
        /// @brief Construct with no default value.
        StrOption() : m_default(pjh::result::Option<std::string>::None()) {}

        /// @brief Whether a default string value has been set.
        bool has_default() const noexcept override { return m_default.is_some(); }

        /// @brief Store the raw string directly (passthrough).
        /// @param ctx Parse context.
        /// @param raw CLI token value (stored as-is).
        /// @return Always Ok.
        CliResult<void> parse_value(
            ParseContext &ctx, std::string_view raw) const override
        {
            ctx.set_value<std::string>(m_key_hash, std::string(raw));
            return CliResult<void>::Ok();
        }

        /// @brief Apply the default string value if @p ctx has none.
        CliResult<void> apply_default(ParseContext &ctx) const override
        {
            if (m_default.is_some() && !ctx.has_value(m_key_hash))
                ctx.set_value<std::string>(m_key_hash, m_default.unwrap());
            return CliResult<void>::Ok();
        }

        /// @brief Register a typed default value.
        /// @param v Default string value.
        /// @return *this for chaining.
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
