#ifndef INCLUDE_PJH_CLI_OPTION_MIXIN_WITH_ENV_HPP
#define INCLUDE_PJH_CLI_OPTION_MIXIN_WITH_ENV_HPP

#include <pjh_cli/option/option_def.hpp>
#include <string>
#include <type_traits>

namespace pjh::cli
{

    /// @brief Mixin: adds environment-variable fallback support.
    ///
    /// When the option is absent on the command line, the parser looks for
    /// an environment variable with the registered name and falls back to
    /// its value.
    template <typename Derived, typename Base = OptionDef>
        requires std::derived_from<Base, OptionDef>
    class WithEnv : public Base
    {
        std::string m_env_var;

    public:
        /// @brief Environment variable name (empty if none).
        const std::string &env_var() const noexcept override { return m_env_var; }

        /// @brief Set the environment variable name for fallback.
        Derived &env(std::string var)
        {
            m_env_var = std::move(var);
            return static_cast<Derived &>(*this);
        }
    };

}  // namespace pjh::cli

#endif
