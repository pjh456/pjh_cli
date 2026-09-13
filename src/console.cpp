#include <cstddef>
#include <memory>
#include <pjh_cli/command/base_command.hpp>
#include <pjh_cli/command/branch_command.hpp>
#include <pjh_cli/console.hpp>
#include <pjh_cli/console/help_navigator.hpp>
#include <pjh_cli/console/history.hpp>
#include <pjh_cli/console/in_memory_history.hpp>
#include <pjh_cli/console/line_editor.hpp>
#include <pjh_cli/console/query_explorer.hpp>
#include <pjh_cli/console/query_output.hpp>
#include <pjh_cli/core/type.hpp>
#include <pjh_cli/detail/tokenizer.hpp>
#include <pjh_cli/format/help_formatter.hpp>
#include <pjh_cli/format/hint.hpp>
#include <pjh_cli/format/info.hpp>
#include <pjh_cli/format/matcher.hpp>
#include <pjh_cli/parse/parser.hpp>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
    /// @brief RAII guard restoring cooked terminal mode around an action.
    ///
    /// Calls suspend() on construction and resume() on destruction, so the
    /// terminal is re-entered even when the action throws.  A null terminal
    /// (piped input / scripted console) makes both calls no-ops.
    class TerminalActionGuard
    {
    public:
        /// @param terminal  Terminal to suspend; may be nullptr.
        explicit TerminalActionGuard(pjh::cli::ITerminal *terminal) : m_terminal(terminal)
        {
            if (m_terminal)
                m_terminal->suspend();
        }

        ~TerminalActionGuard()
        {
            if (m_terminal)
                m_terminal->resume();
        }

        TerminalActionGuard(const TerminalActionGuard &) = delete;
        TerminalActionGuard &operator=(const TerminalActionGuard &) = delete;

    private:
        pjh::cli::ITerminal *m_terminal;
    };

    /// @brief RAII guard making InteractiveConsole::run() re-entrancy-safe.
    ///
    /// Sets the running flag on construction and restores its previous value on
    /// destruction, so a nested run() on the same console cannot leave the outer
    /// loop's flag cleared when it exits.  Restores on exceptions too.
    class RunningGuard
    {
    public:
        /// @param running  Console running flag to set while the scope lives.
        explicit RunningGuard(bool &running) : m_running(running), m_previous(running)
        {
            m_running = true;
        }

        ~RunningGuard() { m_running = m_previous; }

        RunningGuard(const RunningGuard &) = delete;
        RunningGuard &operator=(const RunningGuard &) = delete;

    private:
        bool &m_running;
        bool m_previous;
    };

    /// @brief Strip leading spaces and tabs from a REPL line.
    ///
    /// REPL meta lines (`?` query, quit/exit/q) dispatch on the first
    /// non-blank character, so padding before the keyword must not change
    /// the dispatch path.
    std::string_view ltrim_repl_line(std::string_view line)
    {
        while (!line.empty() && (line.front() == ' ' || line.front() == '\t'))
            line.remove_prefix(1);
        return line;
    }

    /// @brief Strip surrounding spaces and tabs from a REPL meta-line keyword.
    ///
    /// Meta lines dispatch on the keyword alone, so `? save`, `?  save` and
    /// `?save` are the same query, and `quit `/`q\t` exit like their bare form
    /// instead of leaking into command dispatch.
    std::string trim_repl_view(std::string_view view)
    {
        while (!view.empty() && (view.front() == ' ' || view.front() == '\t'))
            view.remove_prefix(1);
        while (!view.empty() && (view.back() == ' ' || view.back() == '\t'))
            view.remove_suffix(1);
        return std::string(view);
    }
}  // namespace

namespace pjh::cli
{

    InteractiveConsole::InteractiveConsole(
        BranchCommand &root,
        std::string prompt,
        std::istream &input,
        std::ostream &output,
        std::ostream &error,
        std::function<std::string(const QueryResult &)> query_fmt,
        std::function<std::string(const HelpNavigationResult &)> help_fmt,
        std::unique_ptr<IHistory> history,
        ErrorFormatterFn error_fmt) :
        m_root(root),
        m_prompt(std::move(prompt)),
        m_input(input),
        m_output(output),
        m_error(error),
        m_query_formatter(
            query_fmt ? std::move(query_fmt) : [](const QueryResult &r)
                { return QueryOutput::format(r); }),
        m_help_formatter(
            help_fmt ? std::move(help_fmt) : [](const HelpNavigationResult &r)
                { return HelpNavigationOutput::format(r); }),
        m_error_formatter(std::move(error_fmt)),
        m_history(history ? std::move(history) : std::make_unique<InMemoryHistory>())
    {
    }

    void InteractiveConsole::run()
    {
        RunningGuard running(m_running);  // saves previous, sets true, restores on exit

        CompletionFn complete = [this](std::string_view line, std::size_t cursor)
        {
            return complete_line_result(m_root, line, cursor);
        };
        HintFn hint = [this](std::string_view line, std::size_t)
        {
            return HintBuilder::format(m_root, line);
        };

        if (!m_terminal)
            m_terminal = make_tty_terminal(m_input, m_output);

        std::string line;
        while (m_running)
        {
            std::shared_ptr<ITerminal> term = m_terminal;  // pin for this line
            if (!term)
                break;  // removed mid-run -> getline fallback

            LineEditor editor(*term, m_prompt, m_history.get());
            if (!editor.read_line(line, complete, hint))
                return;

            // Meta lines dispatch on the trimmed keyword, so ` ?query`,
            // ` quit` and `quit ` behave like their bare form; a blank line
            // stays a no-op.
            std::string_view rest = ltrim_repl_line(line);
            if (rest.empty())
                continue;
            std::string keyword = trim_repl_view(rest);
            if (keyword == "quit" || keyword == "exit" || keyword == "q")
                return;
            auto r = process_line(line);
            if (r.is_err())
                print_error(r.unwrap_err());
        }

        while (m_running)
        {
            m_output << m_prompt << " " << std::flush;

            if (!std::getline(m_input, line))
                break;

            // Meta lines dispatch on the trimmed keyword, so ` ?query`,
            // ` quit` and `quit ` behave like their bare form; a blank line
            // stays a no-op.
            std::string_view rest = ltrim_repl_line(line);
            if (rest.empty())
                continue;

            std::string keyword = trim_repl_view(rest);
            if (keyword == "quit" || keyword == "exit" || keyword == "q")
                break;

            auto r = process_line(line);
            if (r.is_err())
                print_error(r.unwrap_err());
        }
    }

    void InteractiveConsole::stop() { m_running = false; }

    void InteractiveConsole::print_error(const CliError &err)
    {
        if (m_error_formatter)
            m_error << m_error_formatter(err) << "\n";
        else
            m_error << err.what() << "\n";  // byte-identical default
    }

    CliResult<void> InteractiveConsole::handle_query(const std::string &query)
    {
        auto result = QueryExplorer::explore(m_root, query);
        m_output << m_query_formatter(result) << "\n";
        return CliResult<void>::Ok();
    }

    CliResult<void> InteractiveConsole::handle_help(
        const std::vector<std::string> &tokens)
    {
        auto result = HelpNavigator::navigate(m_root, tokens);
        m_output << m_help_formatter(result);
        return CliResult<void>::Ok();
    }

    CliResult<void> InteractiveConsole::process_line(const std::string &line)
    {
        if (line.empty())
            return CliResult<void>::Ok();

        // Record every dispatched non-empty line before any branch runs, so
        // parse-failed and meta (`help` / `?query`) lines are recallable.
        if (m_history)
            m_history->push(line);

        // Skip leading padding before dispatch so callers passing padded lines
        // (e.g. ` ?query`) reach the same branch as the un-padded form.
        std::string_view rest = ltrim_repl_line(line);
        if (rest.empty())
            return CliResult<void>::Ok();

        if (rest.front() == '?')
            return handle_query(trim_repl_view(rest.substr(1)));

        auto tokens = detail::Tokenizer::tokenize(line);
        if (tokens.empty())
            return CliResult<void>::Ok();

        if (tokens[0] == "help" || tokens[0] == "--help" || tokens[0] == "-h")
            return handle_help(tokens);

        std::vector<std::string_view> args;
        for (const auto &t : tokens) args.emplace_back(t);

        auto r = Parser::parse_command(m_root, args, 3, m_root.help_formatter());
        if (r.is_err())
            return CliResult<void>::Err(r.unwrap_err());

        auto &ctx = r.unwrap();

        if (ctx.help_requested())
        {
            m_output << ctx.help_text() << "\n";
            return CliResult<void>::Ok();
        }

        if (ctx.version_requested())
        {
            // version_text() already ends with "\n"; do not append another.
            m_output << ctx.version_text();
            return CliResult<void>::Ok();
        }

        auto *cmd = ctx.matched_command();
        if (!cmd)
            return CliFailure{ErrorFactory::no_command_matched()};

        CliResult<void> exec = CliResult<void>::Ok();
        {
            std::shared_ptr<ITerminal> term = m_terminal;  // pin for the action
            TerminalActionGuard guard(term.get());
            exec = cmd->execute(ctx);
        }
        return exec;
    }

}  // namespace pjh::cli