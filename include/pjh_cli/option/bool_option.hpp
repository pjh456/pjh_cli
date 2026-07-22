#ifndef INCLUDE_PJH_CLI_OPTION_BOOL_OPTION_HPP
#define INCLUDE_PJH_CLI_OPTION_BOOL_OPTION_HPP

#include <pjh_cli/option/mixin/with_default.hpp>
#include <pjh_cli/option/mixin/with_negatable.hpp>

namespace pjh::cli
{

    /// @brief Boolean flag option with optional --no-xxx negation.
    class BoolOption
        : public WithNegatable<bool, BoolOption, WithDefault<bool, BoolOption>>
    {
    };

}  // namespace pjh::cli

#endif
