#ifndef INCLUDE_PJH_CLI_CONSOLE_HPP
#define INCLUDE_PJH_CLI_CONSOLE_HPP

#include <functional>
#include <iostream>
#include <memory>
#include <pjh_cli/command/branch_command.hpp>
#include <pjh_cli/console/history.hpp>
#include <pjh_cli/console/line_editor.hpp>
#include <pjh_cli/core/type.hpp>
#include <pjh_cli/format/info.hpp>
#include <string>
#include <vector>

namespace pjh::cli
{
    struct QueryResult;
    struct HelpNavigationResult;

    /// @brief Renderer for a failed REPL line.
    ///
    /// Receives the full CliError, so a formatter can branch on kind()
    /// (Parse vs Runtime) and inspect info().  The returned string is written
    /// to the console's error stream followed by a newline.  An empty
    /// formatter selects the built-in CliError::what() rendering.
    using ErrorFormatterFn = std::function<std::string(const CliError &)>;

    /// @brief Interactive REPL console for navigating and executing commands.
    ///
    /// Reads lines from an input stream, dispatches them to:
    ///   - `?` / `?query` — list or search subcommands
    ///   - `help` / `--help` / `-h [cmd...]` — display help
    ///   - `--version` / `cmd --version` — print the version text built by the
    ///     parser and skip the matched command's action
    ///   - everything else — parsed as CLI args and executed via action callbacks
    ///
    /// Two distinct help formatters are involved: the `help` / `--help` / `-h`
    /// navigation path uses the injected @p help_fmt (HelpNavigationResult), while
    /// a `cmd --help` line is parsed and rendered by the root command's
    /// help_formatter() (App::set_help_formatter), empty selecting the built-in
    /// renderer.
    ///
    /// A parsed `--version` line writes the parser-built version text verbatim
    /// (it is already newline-terminated) and returns Ok without executing any
    /// action.  Version output does not go through any formatter: neither the
    /// root command's help_formatter() (batch) nor the console's @p help_fmt
    /// participates.
    ///
    /// Failed lines are written to the error stream through an injectable error
    /// formatter (set_error_formatter() or the trailing @p error_fmt constructor
    /// argument).  It receives the full CliError, so it can branch on
    /// ErrorKind::Parse / ErrorKind::Runtime; empty selects the built-in
    /// CliError::what() rendering.
    ///
    /// All three I/O streams are configurable at construction time (defaulting
    /// to std::cin / std::cout / std::cerr), making the console embeddable in
    /// GUI, WebSocket, server, or test contexts without global stream redirection.
    ///
    /// When stdin and stdout are both the process console and both are
    /// interactive TTYs, run() reads through a raw-mode LineEditor: Tab completes
    /// the token under the cursor via
    /// complete_line_result()
    /// (subcommand names, option names, and `.completer` option values).  A unique
    /// candidate is appended in place; zero or several candidates print the
    /// candidate list plus a HintBuilder hint and redraw the prompt.  Up/Down
    /// recall the previous/next line from the injected IHistory, restoring the
    /// draft typed before the first Up when Down passes the newest entry.
    /// Recall covers every submitted non-empty line, including `help` / `?query`
    /// meta lines and lines that fail to parse.
    /// A redirected stdout (e.g. `app > log`) or any injected stream keeps the
    /// line-based std::getline path unchanged and cannot observe arrow keys.  A
    /// custom terminal can be injected with set_terminal(), e.g. a scripted one in
    /// tests.
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
        /// @param root       Root command (typically your App instance).  Must
        ///                   outlive the console.
        /// @param prompt     Prompt string shown before each input line.
        /// @param input      Input stream (default std::cin).
        /// @param output     Output stream (default std::cout).  Also the echo
        ///                   sink for raw-mode editing; must be std::cout for the
        ///                   auto-probe to enter raw mode.
        /// @param error      Error stream (default std::cerr).
        /// @param query_fmt  Query result formatter (default: QueryOutput::format).
        ///                   Replace to customise how query results are rendered.
        /// @param help_fmt   Help navigation formatter (default:
        ///                   HelpNavigationOutput::format).  Replace to customise how
        ///                   help/unknown-subcommand messages are rendered.  This
        ///                   formatter renders the `help` / `--help` / `-h`
        ///                   navigation path only; a `cmd --help` line uses the root
        ///                   command's help_formatter() (App::set_help_formatter).
        /// @param history    Command history implementation (default:
        ///                   InMemoryHistory).  Pass nullptr to disable history,
        ///                   or a NoOpHistory / custom IHistory subclass to
        ///                   override storage.
        /// @param error_fmt  Error formatter for failed lines (default: empty =
        ///                   CliError::what()).  Receives the CliError so it can
        ///                   branch on ErrorKind (Parse vs Runtime).  Trailing after
        ///                   history for source compatibility; set_error_formatter()
        ///                   is the ergonomic install path.
        explicit InteractiveConsole(
            BranchCommand &root,
            std::string prompt = "> ",
            std::istream &input = std::cin,
            std::ostream &output = std::cout,
            std::ostream &error = std::cerr,
            std::function<std::string(const QueryResult &)> query_fmt = {},
            std::function<std::string(const HelpNavigationResult &)> help_fmt = {},
            std::unique_ptr<IHistory> history = {},
            ErrorFormatterFn error_fmt = {});

        /// @brief Run the REPL loop.  Blocks until EOF, "quit", "exit",
        ///        "q" (matched case-insensitively), or stop() is called from a
        ///        callback.
        ///
        /// Re-entrant on the same console: an action may call run() again, and
        /// the outer invocation's running state is restored when the nested loop
        /// returns (EOF, "quit"/"exit"/"q", or stop()), so the outer loop
        /// continues.  stop() from a nested invocation stops only the innermost
        /// active loop.
        ///
        /// The input path is chosen per line: when both stdin and stdout are
        /// interactive TTYs — or a terminal is installed via set_terminal() —
        /// input is read through a LineEditor that handles Tab completion, hint
        /// rendering, and Up/Down recall from the injected IHistory.  m_terminal
        /// is re-resolved and pinned once per line on both input paths, so
        /// set_terminal() (or nullptr) may be called from an action: the current
        /// line finishes on the input source it was read on and the replacement
        /// takes effect on the next line.  On the LineEditor path Ctrl-C cancels
        /// the current line (buffer discarded, `^C` echoed, fresh prompt) and the
        /// REPL keeps running; Ctrl-D or `quit`/`exit`/`q` (case-insensitive)
        /// end it.  Otherwise each iteration:
        ///   1. Prints @p m_prompt to m_output verbatim (no separator is
        ///      appended; include any trailing space in the prompt).
        ///   2. Reads a line from m_input with std::getline (arrow keys are
        ///      consumed by the terminal line discipline and cannot navigate).
        ///   3. Skips empty lines.
        ///   4. Exits on "quit" / "exit" / "q" (case-insensitive).
        ///   5. Calls process_line() and prints errors to m_error.
        void run();

        /// @brief Signal the innermost active run() to exit gracefully on the
        ///        next iteration.
        ///
        /// The currently running iteration completes normally, then the
        /// next iteration sees m_running == false and exits.  When run() is
        /// nested on the same console, only the innermost active invocation is
        /// stopped; after it returns, the outer loop resumes unless the outer
        /// action calls stop() as well.
        void stop();

        /// @brief Get the current prompt string.
        /// @return Const reference to the prompt, e.g. `"> "`.
        const std::string &prompt() const noexcept { return m_prompt; }

        /// @brief Override the prompt string.
        /// @param p  New prompt (e.g. `"$ "`).
        void set_prompt(std::string p) { m_prompt = std::move(p); }

        /// @brief Install a custom terminal (e.g. a scripted one in tests).
        ///        Pass nullptr to fall back to TTY detection / std::getline.
        ///
        /// Safe to call while run() or process_line() is executing: the
        /// terminal currently in use is kept alive until the in-flight read or
        /// action finishes, and the replacement takes effect on the next line
        /// read or the next process_line() call.  This holds whether run() is
        /// currently on the LineEditor path or the std::getline fallback:
        /// passing nullptr mid-run() makes run() switch to the line-based
        /// std::getline loop on the next line.
        /// Not thread-safe: call from the same thread as run()/process_line().
        /// @param terminal  Raw-mode terminal implementation; ownership taken.
        void set_terminal(std::unique_ptr<ITerminal> terminal)
        {
            m_terminal = std::move(terminal);
        }

        /// @brief Install a custom error formatter (empty = CliError::what()).
        ///
        /// Read on every failed line, so installing or clearing it while run() is
        /// active takes effect on the next error.  Pass {} to restore the built-in
        /// CliError::what() rendering.
        /// @param formatter  Renderer, or {} for the built-in default.
        void set_error_formatter(ErrorFormatterFn formatter)
        {
            m_error_formatter = std::move(formatter);
        }

        /// @brief The current error formatter (empty = built-in CliError::what()).
        const ErrorFormatterFn &error_formatter() const noexcept
        {
            return m_error_formatter;
        }

        /// @brief Parse and execute a single line of input.
        ///
        /// Dispatches based on the first token:
        ///   - `?query`  → handle_query() — list or search subcommands
        ///   - `help` / `--help` / `-h`  → handle_help() — display help
        ///     for a subcommand chain
        ///   - default   → Parser::parse_command() with max_fuzzy 3,
        ///                  then execute the matched command's action callback
        ///
        /// A `cmd --help` line is rendered by the root command's help_formatter()
        /// (empty selects the built-in), so App::set_help_formatter governs it.
        ///
        /// A `--version` line (including `cmd --version`, where the parser
        /// reports the root version) consumes version_requested() before the
        /// matched action runs: the newline-terminated version_text() is written
        /// to m_output verbatim and Ok is returned.  This path is independent of
        /// App::set_help_formatter and of the console's navigation formatter.
        ///
        /// The line is appended to the injected IHistory before dispatch, so a
        /// parse failure or a meta (`help` / `?query` / `cmd --help`) line is
        /// still recallable; empty and consecutive-duplicate filtering is the
        /// backend's contract.  Recording never changes the return value.
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

        /// @brief Configurable help navigation formatter.
        ///
        /// Defaults to HelpNavigationOutput::format.  Override to change
        /// how help text, "unknown subcommand", and "has no subcommands"
        /// messages are rendered without changing the navigation logic.
        /// This is the `help` / `--help` / `-h` navigation formatter only; the
        /// `cmd --help` path uses the root command's help_formatter().
        std::function<std::string(const HelpNavigationResult &)> m_help_formatter;

        /// @brief Configurable error formatter (empty = CliError::what()).
        ///
        /// Applied to every failed line in run(); the formatter receives the full
        /// CliError and may branch on kind().  Empty selects what().
        ErrorFormatterFn m_error_formatter;

        /// @brief Whether the REPL loop should continue running.
        ///
        /// Saved and restored per run() invocation by a function-local RAII
        /// guard, so nested run() calls on the same console do not clobber the
        /// outer loop's state.
        bool m_running = false;

        /// @brief Command history storage.
        ///
        /// Defaults to InMemoryHistory.  Use NoOpHistory or nullptr to
        /// disable history injection, or inject a custom IHistory subclass
        /// to override storage backend.
        /// push() is called once at the top of process_line() for every non-empty
        /// line before dispatch, so `?`/`help` meta lines, `cmd --help`, parse
        /// failures and execution failures are all recorded.  Empty lines and the
        /// `quit`/`exit`/`q` loop terminators (consumed by run()) are not.
        std::unique_ptr<IHistory> m_history;

        /// @brief Optional raw-mode terminal.  Shared ownership keeps a
        ///        terminal alive while a line read or an action still uses it;
        ///        run() re-reads this once per line on both input paths, so a
        ///        replacement is picked up on the next line / process_line().
        ///        When unset, run() probes the input stream once at entry and
        ///        falls back to std::getline for non-TTYs.
        std::shared_ptr<ITerminal> m_terminal;

        /// @brief Handle `?` or `?query` — list or search subcommands.
        ///
        /// Delegates to QueryExplorer::explore() for data collection,
        /// then renders through m_query_formatter to m_output.
        /// Does not write to any stream directly.
        ///
        /// @param query  The search string (without the leading `?` or
        ///              surrounding padding).
        /// @return Ok() after printing results to m_output.
        CliResult<void> handle_query(const std::string &query);

        /// @brief Render a failed line to m_error through m_error_formatter.
        ///
        /// Writes m_error_formatter ? m_error_formatter(err) : err.what(),
        /// followed by a newline.  Never called by process_line(): run() owns
        /// error printing so process_line stays a pure producer.
        /// @param err  Error returned by process_line().
        void print_error(const CliError &err);

        /// @brief Handle `help`, `--help`, or `-h [subcommand...]`.
        ///
        /// Delegates to HelpNavigator::navigate() for data collection,
        /// then renders through m_help_formatter to m_output.
        /// Does not write to any stream directly.
        ///
        /// @param tokens  Tokenised input line (tokens[0] is "help"/"--help"/"-h").
        /// @return Ok() after printing help text or error messages to m_output.
        CliResult<void> handle_help(const std::vector<std::string> &tokens);
    };

}  // namespace pjh::cli

#endif