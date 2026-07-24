#ifndef INCLUDE_PJH_CLI_CONSOLE_HPP
#define INCLUDE_PJH_CLI_CONSOLE_HPP

#include <functional>
#include <iostream>
#include <pjh_cli/command/branch_command.hpp>
#include <pjh_cli/core/type.hpp>
#include <pjh_cli/format/info.hpp>
#include <string>
#include <vector>

namespace pjh::cli
{
    struct QueryResult;
    /// @brief Interactive REPL console for navigating and executing commands.
    ///
    /// Reads lines from an input stream, dispatches them to:
    ///   - `?` / `?query` — list or search subcommands
    ///   - `help` / `--help` / `-h [cmd...]` — display help
    ///   - everything else — parsed as CLI args and executed via action callbacks
    ///
    /// All three I/O streams are configurable at construction time (defaulting
    /// to std::cin / std::cout / std::cerr), making the console embeddable in
    /// GUI, WebSocket, server, or test contexts without global stream redirection.
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
        /// @param root    Root command (typically your App instance).  Must
        ///                outlive the console.
        /// @param prompt  Prompt string shown before each input line.
        /// @param input   Input stream (default std::cin).
        /// @param output  Output stream (default std::cout).
        /// @param error   Error stream (default std::cerr).
        /// @param query_fmt  Query result formatter (default: QueryOutput::format).
        ///                Replace to customise how query results are rendered.
        explicit InteractiveConsole(
            BranchCommand &root,
            std::string prompt = "> ",
            std::istream &input = std::cin,
            std::ostream &output = std::cout,
            std::ostream &error = std::cerr,
            std::function<std::string(const QueryResult &)> query_fmt = {});

        /// @brief Run the REPL loop.  Blocks until EOF, "quit", "exit",
        ///        "q", or stop() is called from a callback.
        ///
        /// Each iteration:
        ///   1. Prints @p m_prompt to m_output.
        ///   2. Reads a line from m_input.
        ///   3. Skips empty lines.
        ///   4. Exits on "quit" / "exit" / "q".
        ///   5. Calls process_line() and prints errors to m_error.
        void run();

        /// @brief Signal the loop to exit gracefully on the next iteration.
        ///
        /// The currently running iteration completes normally, then the
        /// next iteration sees m_running == false and exits.
        void stop();

        /// @brief Get the current prompt string.
        /// @return Const reference to the prompt, e.g. `"> "`.
        const std::string &prompt() const noexcept { return m_prompt; }

        /// @brief Override the prompt string.
        /// @param p  New prompt (e.g. `"$ "`).
        void set_prompt(std::string p) { m_prompt = std::move(p); }

        /// @brief Parse and execute a single line of input.
        ///
        /// Dispatches based on the first token:
        ///   - `?query`  → handle_query() — list or search subcommands
        ///   - `help` / `--help` / `-h`  → handle_help() — display help
        ///     for a subcommand chain
        ///   - default   → Parser::parse_command() with max_fuzzy 3,
        ///                  then execute the matched command's action callback
        ///
        /// @param line  Raw input line (may be empty, in which case Ok is returned).
        /// @return Ok() on success, or Err(CliError) if parsing or the
        ///         action callback fails.
        CliResult<void> process_line(const std::string &line);

    private:
        /// @brief Root of the command tree.  Must outlive this console.
        BranchCommand &m_root;

        /// @brief Prompt displayed before each input line.
        std::string m_prompt;

        /// @brief Configurable input stream (defaults to std::cin).
        std::istream &m_input;

        /// @brief Configurable output stream (defaults to std::cout).
        std::ostream &m_output;

        /// @brief Configurable error stream (defaults to std::cerr).
        std::ostream &m_error;

        /// @brief Configurable query result formatter.
        ///
        /// Defaults to QueryOutput::format.  Override to change how
        /// ?-query results are rendered without changing the exploration
        /// logic (substring matching, fuzzy fallback).
        std::function<std::string(const QueryResult &)> m_query_formatter;

        /// @brief Whether the REPL loop should continue running.
        bool m_running = false;

        /// @brief Ring buffer of previously entered lines (for history navigation).
        ///
        /// Currently declared but not wired into the REPL loop.
        /// Planned for Phase D (IHistory interface).
        std::vector<std::string> m_history;

        /// @brief Current index into m_history for up/down arrow navigation.
        ///
        /// A value of m_history.size() means "at the end" (current input).
        size_t m_history_index = 0;

        /// @brief Handle `?` or `?query` — list or search subcommands.
        ///
        /// Delegates to QueryExplorer::explore() for data collection,
        /// then renders through m_query_formatter to m_output.
        /// Does not write to any stream directly.
        ///
        /// @param query  The search string (without the leading `?`).
        /// @return Ok() after printing results to m_output.
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
        /// @return Ok() after printing help text or error messages to m_output.
        CliResult<void> handle_help(const std::vector<std::string> &tokens);

        /// @brief Fuzzy-find subcommand names under @p branch.
        ///
        /// Wraps fuzzy_find_subcommands() with max_distance = 3 and
        /// Visibility::Repl, then maps the result into SuggestionInfo
        /// suitable for ConsoleOutput::format_suggestions().
        ///
        /// @param branch  The parent branch to search under.
        /// @param input   User input to match (mangled or abbreviated name).
        /// @return SuggestionInfo with matches sorted by distance.
        static SuggestionInfo collect_fuzzy_suggestions(
            BranchCommand &branch, std::string_view input);
    };

}  // namespace pjh::cli

#endif