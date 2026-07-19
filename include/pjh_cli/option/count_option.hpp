#ifndef INCLUDE_PJH_CLI_OPTION_COUNT_OPTION_HPP
#define INCLUDE_PJH_CLI_OPTION_COUNT_OPTION_HPP

#include "../option_def.hpp"
#include "../parse_context.hpp"
#include "int_option.hpp"

namespace pjh::cli
{
    /// @brief Counting-flag option (-vvv → 3), inherits IntOption.
    ///
    /// Created by `OptionBuilder::count()`.  Unlike IntOption, a CountOption
    /// never consumes a CLI value token, never has a default value, and each
    /// occurrence increments the stored int by 1.  Accessible via
    /// `ctx.get<int, "key">()` just like a normal int option.
    class CountOption : public IntOption
    {
    public:
        /// @brief Construct with counting mode enabled, no value consumption.
        CountOption() { m_has_value = false; }

        /// @brief Counting flags never consume a value token.
        bool has_value() const noexcept override { return false; }

        /// @brief Counting flags never have a default value.
        bool has_default() const noexcept override { return false; }

        /// @brief Counting is always enabled.
        bool is_counting() const noexcept override { return true; }

        /// @brief Should never be called (has_value is false).
        CliResult<void> parse_value(ParseContext &, std::string_view) const override
        {
            return CliFailure{CliError("counting option does not accept a value")};
        }

        /// @brief Counting has no default to apply.
        CliResult<void> apply_default(ParseContext &) const override
        {
            return CliResult<void>::Ok();
        }

        /// @brief Explicitly prevent setting a default on counting flags.
        IntOption &default_value(int v) = delete;
    };

}  // namespace pjh::cli

#endif  // INCLUDE_PJH_CLI_OPTION_COUNT_OPTION_HPP
