#ifndef INCLUDE_PJH_CLI_OPTION_FLOAT_OPTION_HPP
#define INCLUDE_PJH_CLI_OPTION_FLOAT_OPTION_HPP

#include <pjh_cli/option/mixin/with_default.hpp>
#include <pjh_cli/option/mixin/with_range.hpp>

namespace pjh::cli
{

    /// @brief Double-valued floating-point option with min/max range validation.
    class FloatOption
        : public WithRange<double, FloatOption, WithDefault<double, FloatOption>>
    {
    };

}  // namespace pjh::cli

#endif
