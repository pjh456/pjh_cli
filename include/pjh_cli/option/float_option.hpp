#ifndef INCLUDE_PJH_CLI_OPTION_FLOAT_OPTION_HPP
#define INCLUDE_PJH_CLI_OPTION_FLOAT_OPTION_HPP

#include <pjh_cli/detail/option_chain.hpp>
#include <pjh_cli/option/mixin/with_default.hpp>
#include <pjh_cli/option/mixin/with_env.hpp>
#include <pjh_cli/option/mixin/with_range.hpp>
#include <pjh_cli/option/mixin/with_repeatable.hpp>
#include <pjh_cli/option/mixin/with_required.hpp>

namespace pjh::cli
{

    /// @brief Double-valued floating-point option with min/max range validation.
    class FloatOption : public detail::option_chain<
                            double,
                            FloatOption,
                            WithRequired,
                            WithEnv,
                            WithRange,
                            WithRepeatable,
                            WithDefault>
    {
    };

}  // namespace pjh::cli

#endif
