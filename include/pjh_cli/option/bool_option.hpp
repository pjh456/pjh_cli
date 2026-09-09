#ifndef INCLUDE_PJH_CLI_OPTION_BOOL_OPTION_HPP
#define INCLUDE_PJH_CLI_OPTION_BOOL_OPTION_HPP
#include <pjh_cli/core/converter.hpp>
#include <pjh_cli/detail/option_chain.hpp>
#include <pjh_cli/option/mixin/with_default.hpp>
#include <pjh_cli/option/mixin/with_env.hpp>
#include <pjh_cli/option/mixin/with_negatable.hpp>
#include <pjh_cli/option/mixin/with_required.hpp>
#include <string_view>

namespace pjh::cli
{

    /// @brief Boolean flag option with optional --no-xxx negation.
    class BoolOption : public detail::option_chain<
                           bool,
                           BoolOption,
                           WithRequired,
                           WithEnv,
                           WithNegatable,
                           WithDefault>
    {
    protected:
        CliResult<bool> convert_value(std::string_view raw) const override
        {
            return Converter<bool>::from_string(raw);
        }
    };

}  // namespace pjh::cli

#endif
