#ifndef INCLUDE_PJH_CLI_CONSOLE_HPP
#define INCLUDE_PJH_CLI_CONSOLE_HPP

#include "command.hpp"

#include <string>
#include <vector>

namespace pjh::cli
{
    /// @brief Interactive REPL console for navigating and executing commands.
    class InteractiveConsole
    {
    public:
        /// @param root  Root command (typically your App instance).
        /// @param prompt  Prompt string shown before each input.
        explicit InteractiveConsole(
            Command &root,
            std::string prompt = "> ");

        /// @brief Run the REPL loop. Blocks until exit or EOF.
        void run();

        /// @brief Signal the loop to exit gracefully.
        void stop();

        const std::string &
        prompt() const noexcept { return m_prompt; }

        void
        set_prompt(std::string p) { m_prompt = std::move(p); }

    private:
        Command &m_root;
        std::string m_prompt;
        bool m_running = false;
        std::vector<std::string> m_history;
        size_t m_history_index = 0;

        void process_line(
            const std::string &line);
    };

} // namespace pjh::cli

#endif
