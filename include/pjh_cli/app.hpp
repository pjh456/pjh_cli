#ifndef INCLUDE_PJH_CLI_APP_HPP
#define INCLUDE_PJH_CLI_APP_HPP

#include "command.hpp"

namespace pjh::cli
{
    /// @brief Application entry point — the root command.
    ///
    /// Inherits Command and adds version metadata, parse(), and run_interactive().
    class App final : public Command
    {
    public:
        /// @brief Construct an application root.
        /// @param name        Binary / application name.
        /// @param version     Version string.
        /// @param description Short description shown in help.
        App(
            std::string name,
            std::string version,
            std::string description);

        /// @brief The version string.
        const std::string &
        version() const { return m_version; }

    private:
        std::string m_version;
    };

} // namespace pjh::cli

#endif // INCLUDE_PJH_CLI_APP_HPP
