#ifndef INCLUDE_PJH_CLI_CONSOLE_HPP
#define INCLUDE_PJH_CLI_CONSOLE_HPP

#include <string>
#include <vector>

#include "command/branch_command.hpp"
#include "type.hpp"

namespace pjh::cli
{
    /// @brief Interactive REPL console for navigating and executing commands.
    ///
    /// Reads lines from stdin, dispatches them to:
    ///   - `?` / `?query` — list or search subcommands
    ///   - `help` / `--help` / `-h [cmd...]` — display help
    ///   - everything else — parsed as CLI args and executed via action callbacks
    ///
    /// The console does not own the command tree; the caller must keep the
    /// root BranchCommand alive for the console's lifetime.
    ///
    /// Usage:
    /// @code
    ///   App app("myapp", "1.0", "My app");
    ///   InteractiveConsole console(app, "> ");
    ///   console.run();
    /// @endcode
    class InteractiveConsole
    {
    public:
        /// @param root   Root command (typically your App instance).  Must
        ///               outlive the console.
        /// @param prompt Prompt string shown before each input line.
        explicit InteractiveConsole(BranchCommand &root, std::string prompt = "> ");

        /// @brief Run the REPL loop.  Blocks until EOF, "quit", "exit",
        ///        "q", or stop() is called from a callback.
        ///
        /// Each iteration:
        ///   1. Prints @p m_prompt.
        ///   2. Reads a line from stdin.
        ///   3. Skips empty lines.
        ///   4. Exits on "quit" / "exit" / "q".
        ///   5. Calls process_line() and prints errors to stderr.
        void run();

        /// @brief Signal the loop to exit gracefully on the next iteration.
        void stop();

        /// @brief Current prompt string.
        const std::string &prompt() const noexcept { return m_prompt; }

        /// @brief Override the prompt string.
        void set_prompt(std::string p) { m_prompt = std::move(p); }

        /// @brief Parse and execute a single line of input.
        ///
        /// Dispatches based on the first token:
        ///   - `?query`  → handle_query()
        ///   - `help` / `--help` / `-h`  → handle_help()
        ///   - default   → Parser::parse_command() with max_fuzzy 3,
        ///                  then execute the matched command's action callback
        ///
        /// @param line  Raw input line (may be empty, in which case Ok is returned).
        /// @return Ok() or Err(CliError) from parse or from the action callback.
        CliResult<void> process_line(const std::string &line);

    private:
        BranchCommand &m_root;
        std::string m_prompt;
        bool m_running = false;
        std::vector<std::string> m_history;
        size_t m_history_index = 0;

        /// @brief Handle `?` or `?query` — list or search subcommands.
        ///
        /// If @p query is empty, prints all visible/REPL subcommand names.
        /// If non-empty, searches by substring (case-sensitive, enabled/visible
        /// only).  On no substring match, falls back to fuzzy (Levenshtein)
        /// suggestions or prints "No matches." with a usage hint.
        ///
        /// @param query  The search string (without the leading `?`).
        /// @return Ok() after printing results to stdout.
        CliResult<void> handle_query(const std::string &query);

        /// @brief Handle `help`, `--help`, or `-h [subcommand...]`.
        ///
        /// Without arguments, prints full help for the root command.
        /// With arguments, walks the subcommand chain: each token is resolved
        /// via find_subcommand() or fuzzy fallback with "Did you mean:"
        /// suggestions.  Prints the final matched command's help, or an
        /// error if a subcommand is not found.
        ///
        /// @param tokens  Tokenised input line (tokens[0] is "help"/"--help"/"-h").
        /// @return Ok() after printing help text or error messages to stdout.
        CliResult<void> handle_help(const std::vector<std::string> &tokens);

        /// @brief Fuzzy-find subcommand names under @p branch and format them
        ///        as a space-separated string: `" cmd1 cmd2"`.
        ///
        /// Uses fuzzy_find_subcommands() with max_distance = 3 and
        /// Visibility::Repl.  Returns empty string if no fuzzy match is found.
        ///
        /// @param branch  The parent branch to search under.
        /// @param input   User input to match (mangled or abbreviated name).
        /// @return Space-separated string prefixed with a single space,
        ///         or empty.
        static std::string format_fuzzy_suggestions(
            BranchCommand &branch, std::string_view input);
    };

}  // namespace pjh::cli

#endif
