#ifndef INCLUDE_PJH_CLI_OPTION_STR_OPTION_HPP
#define INCLUDE_PJH_CLI_OPTION_STR_OPTION_HPP

#include <pjh_cli/detail/option_chain.hpp>
#include <pjh_cli/option/mixin/with_default.hpp>
#include <pjh_cli/option/mixin/with_env.hpp>
#include <pjh_cli/option/mixin/with_repeatable.hpp>
#include <pjh_cli/option/mixin/with_required.hpp>
#include <string>
#include <string_view>

namespace pjh::cli
{

    /// @brief String-valued option.
    class StrOption : public detail::option_chain<
                          std::string,
                          StrOption,
                          WithRequired,
                          WithEnv,
                          WithRepeatable,
                          WithDefault>
    {
    protected:
        CliResult<std::string> convert_value(std::string_view raw) const override
        {
            return CliResult<std::string>::Ok(std::string(raw));
        }
    };

}  // namespace pjh::cli

#endif
