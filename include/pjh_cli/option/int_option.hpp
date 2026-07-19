#ifndef INCLUDE_PJH_CLI_OPTION_INT_OPTION_HPP
#define INCLUDE_PJH_CLI_OPTION_INT_OPTION_HPP

#include "../converter.hpp"
#include "../option_def.hpp"
#include "../parse_context.hpp"

namespace pjh::cli
{
    /// @brief Integer-valued option.
    ///
    /// Created by `OptionBuilder::integer()`.  Overrides `parse_value()` and
    /// `apply_default()` to store values as `int` in `ParseContext`.
    class IntOption : public OptionDef
    {
    public:
        /// @brief Construct with no default value.
        IntOption() : m_default(pjh::result::Option<int>::None()) {}

        /// @brief Whether a default int value has been set.
        bool has_default() const noexcept override { return m_default.is_some(); }

        /// @brief Parse the raw string as an int and store in @p ctx.
        CliResult<void> parse_value(
            ParseContext &ctx, std::string_view raw) const override
        {
            auto r = Converter<int>::from_string(raw);
            if (r.is_err())
                return CliResult<void>::Err(std::move(r).unwrap_err());
            ctx.set_value<int>(m_key_hash, r.unwrap());
            return CliResult<void>::Ok();
        }

        /// @brief Apply the default int value if @p ctx has none.
        CliResult<void> apply_default(ParseContext &ctx) const override
        {
            if (m_default.is_some() && !ctx.has_value(m_key_hash))
                ctx.set_value<int>(m_key_hash, m_default.unwrap());
            return CliResult<void>::Ok();
        }

        /// @brief Register a typed default value.
        /// @param v Default int value.
        /// @return *this for chaining.
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
