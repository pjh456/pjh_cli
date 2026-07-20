#ifndef INCLUDE_PJH_CLI_CONSOLE_HPP
#define INCLUDE_PJH_CLI_CONSOLE_HPP

#include <string>
#include <vector>

#include "command/branch_command.hpp"
#include "type.hpp"

namespace pjh::cli
{
    /// @brief Interactive REPL console for navigating and executing commands.
    class InteractiveConsole
    {
    public:
        /// @param root  Root command (typically your App instance, must be a BranchCommand).
        /// @param prompt  Prompt string shown before each input.
        explicit InteractiveConsole(BranchCommand &root, std::string prompt = "> ");

        /// @brief Run the REPL loop. Blocks until exit or EOF.
        void run();

        /// @brief Signal the loop to exit gracefully.
        void stop();

        const std::string &prompt() const noexcept { return m_prompt; }

        void set_prompt(std::string p) { m_prompt = std::move(p); }

        CliResult<void> process_line(const std::string &line);

    private:
        BranchCommand &m_root;
        std::string m_prompt;
        bool m_running = false;
        std::vector<std::string> m_history;
        size_t m_history_index = 0;
    };

}  // namespace pjh::cli

#endif
