#ifndef INCLUDE_PJH_CLI_OPTION_COUNT_OPTION_HPP
#define INCLUDE_PJH_CLI_OPTION_COUNT_OPTION_HPP

#include <string_view>

#include <pjh_cli/option/option_def.hpp>
#include <pjh_cli/parse/parse_context.hpp>
#include <pjh_cli/core/error.hpp>
#include <pjh_cli/core/type.hpp>

namespace pjh::cli
{

    /// @brief Counting-flag option (-vvv → 3).
    ///
    /// Never consumes a CLI value token, never has a default value.
    /// Each occurrence increments the stored int by 1.
    /// Accessible via `ctx.get<int, "key">()`.
    class CountOption : public OptionDef
    {
    public:
        bool has_value() const noexcept override { return false; }

        bool has_default() const noexcept override { return false; }

        bool is_counting() const noexcept override { return true; }

        CliResult<void> parse_value(ParseContext &, std::string_view) const override
        {
            return CliFailure{CliError("counting option does not accept a value")};
        }

        CliResult<void> apply_default(ParseContext &) const override
        {
            return CliResult<void>::Ok();
        }
    };

}  // namespace pjh::cli

#endif
