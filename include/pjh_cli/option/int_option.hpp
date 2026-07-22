#ifndef INCLUDE_PJH_CLI_OPTION_INT_OPTION_HPP
#define INCLUDE_PJH_CLI_OPTION_INT_OPTION_HPP

#include <pjh_cli/option/mixin/with_default.hpp>
#include <pjh_cli/option/mixin/with_range.hpp>
#include <pjh_cli/option/mixin/with_repeatable.hpp>

namespace pjh::cli
{

    /// @brief Integer-valued option with min/max range validation.
    class IntOption : public WithRange<
                          int,
                          IntOption,
                          WithRepeatable<IntOption, WithDefault<int, IntOption>>>
    {
    };

}  // namespace pjh::cli

#endif
