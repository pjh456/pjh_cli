#ifndef INCLUDE_PJH_CLI_OPTION_STR_OPTION_HPP
#define INCLUDE_PJH_CLI_OPTION_STR_OPTION_HPP

#include <string>
#include <string_view>

#include "mixin/with_choices.hpp"
#include "mixin/with_default.hpp"

namespace pjh::cli
{

    /// @brief String-valued option with optional choices validation.
    class StrOption
        : public WithChoices<std::string, StrOption, WithDefault<std::string, StrOption>>
    {
    protected:
        CliResult<std::string> convert_value(std::string_view raw) const override
        {
            return CliResult<std::string>::Ok(std::string(raw));
        }
    };

}  // namespace pjh::cli

#endif
