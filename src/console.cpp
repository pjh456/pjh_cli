#include <cstddef>
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
        std::unique_ptr<IHistory> history) :
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
        m_history(history ? std::move(history) : std::make_unique<InMemoryHistory>())
    {
    }

    void InteractiveConsole::run()
    {
        m_running = true;

        CompletionFn complete = [this](std::string_view line, std::size_t cursor)
        {
            return complete_line_result(m_root, line, cursor);
        };
        HintFn hint = [this](std::string_view line, std::size_t)
        {
            return HintBuilder::format(m_root, line);
        };

        std::unique_ptr<ITerminal> owned;
        ITerminal *term = m_terminal.get();
        if (!term)
        {
            owned = make_tty_terminal(m_input, m_output);
            term = owned.get();
        }

        std::string line;
        if (term)
        {
            LineEditor editor(*term, m_prompt, m_history.get());
            while (m_running)
            {
                if (!editor.read_line(line, complete, hint))
                    break;
                if (line.empty())
                    continue;
                if (line == "quit" || line == "exit" || line == "q")
                    break;
                auto r = process_line(line);
                if (r.is_err())
                    m_error << r.unwrap_err().what() << "\n";
            }
            return;
        }

        while (m_running)
        {
            m_output << m_prompt << " " << std::flush;

            if (!std::getline(m_input, line))
                break;

            if (line.empty())
                continue;

            if (line == "quit" || line == "exit" || line == "q")
                break;

            auto r = process_line(line);
            if (r.is_err())
                m_error << r.unwrap_err().what() << "\n";
        }
    }

    void InteractiveConsole::stop() { m_running = false; }

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

        if (line[0] == '?')
            return handle_query(line.substr(1));

        auto tokens = detail::Tokenizer::tokenize(line);
        if (tokens.empty())
            return CliResult<void>::Ok();

        if (tokens[0] == "help" || tokens[0] == "--help" || tokens[0] == "-h")
            return handle_help(tokens);

        std::vector<std::string_view> args;
        for (const auto &t : tokens) args.emplace_back(t);

        auto r = Parser::parse_command(m_root, args, 3);
        if (r.is_err())
            return CliResult<void>::Err(r.unwrap_err());

        auto &ctx = r.unwrap();

        if (ctx.help_requested())
        {
            m_output << ctx.help_text() << "\n";
            return CliResult<void>::Ok();
        }

        auto *cmd = ctx.matched_command();
        if (!cmd)
            return CliFailure{ErrorFactory::no_command_matched()};

        auto exec = cmd->execute(ctx);
        if (m_history)
            m_history->push(line);
        return exec;
    }

}  // namespace pjh::cli