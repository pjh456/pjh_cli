#include <cstddef>
#include <iostream>
#include <pjh_cli/command/base_command.hpp>
#include <pjh_cli/command/branch_command.hpp>
#include <pjh_cli/console.hpp>
#include <pjh_cli/core/type.hpp>
#include <pjh_cli/detail/tokenizer.hpp>
#include <pjh_cli/format/help_formatter.hpp>
#include <pjh_cli/format/matcher.hpp>
#include <pjh_cli/parse/parser.hpp>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace pjh::cli
{
    InteractiveConsole::InteractiveConsole(BranchCommand &root, std::string prompt) :
        m_root(root), m_prompt(std::move(prompt))
    {
    }

    void InteractiveConsole::run()
    {
        m_running = true;
        std::string line;

        while (m_running)
        {
            std::cout << m_prompt << " " << std::flush;

            if (!std::getline(std::cin, line))
                break;

            if (line.empty())
                continue;

            if (line == "quit" || line == "exit" || line == "q")
                break;

            auto r = process_line(line);
            if (r.is_err())
                std::cerr << r.unwrap_err().what() << "\n";
        }
    }

    void InteractiveConsole::stop() { m_running = false; }

    std::string InteractiveConsole::format_fuzzy_suggestions(
        BranchCommand &branch, std::string_view input)
    {
        auto fuzzy = fuzzy_find_subcommands(branch, input, 3, Visibility::Repl);
        if (fuzzy.empty())
            return {};

        std::string result;
        for (const auto &f : fuzzy) result += " " + f.command->name();
        return result;
    }

    CliResult<void> InteractiveConsole::handle_query(const std::string &query)
    {
        if (query.empty())
        {
            auto names = list_subcommands(m_root);
            if (names.empty())
                std::cout << "No subcommands available.\n";
            else
            {
                std::cout << "Subcommands:";
                for (const auto &n : names) std::cout << " " << n;
                std::cout << "\n";
            }
            return CliResult<void>::Ok();
        }

        bool found = false;
        for (const auto &sub_ptr : m_root.subcommands())
        {
            if (!sub_ptr->is_enabled())
                continue;
            if (sub_ptr->visibility() == Visibility::Hidden)
                continue;
            if (sub_ptr->name().find(query) != std::string_view::npos)
            {
                if (!found)
                {
                    std::cout << "Matching subcommands:";
                    found = true;
                }
                std::cout << " " << sub_ptr->name();
                continue;
            }
            for (const auto &a : sub_ptr->aliases())
                if (a.find(query) != std::string_view::npos)
                {
                    if (!found)
                    {
                        std::cout << "Matching subcommands:";
                        found = true;
                    }
                    std::cout << " " << sub_ptr->name();
                    break;
                }
        }

        if (!found)
        {
            auto suggestions = format_fuzzy_suggestions(m_root, query);
            if (!suggestions.empty())
                std::cout << "Did you mean:" << suggestions;
            else
            {
                std::cout << "No matches.";
                std::cout << " Try: "
                          << HelpFormatter::format_usage(m_root, m_root.name());
            }
        }
        std::cout << "\n";
        return CliResult<void>::Ok();
    }

    CliResult<void> InteractiveConsole::handle_help(
        const std::vector<std::string> &tokens)
    {
        if (tokens.size() == 1)
        {
            std::cout << HelpFormatter::format_help(m_root, m_root.name());
            return CliResult<void>::Ok();
        }

        BaseCommand *target = &m_root;
        for (size_t i = 1; i < tokens.size(); i++)
        {
            if (!target->is_branch())
            {
                std::cout << "'" << target->name() << "' has no subcommands.\n";
                return CliResult<void>::Ok();
            }

            auto *branch = target->as_branch();
            auto *sub = branch->find_subcommand(tokens[i]);
            if (!sub)
            {
                auto suggestions = format_fuzzy_suggestions(*branch, tokens[i]);
                if (!suggestions.empty())
                {
                    std::cout << "Unknown subcommand '" << tokens[i]
                              << "'. Did you mean:" << suggestions << "\n";
                }
                else
                {
                    std::cout << "Unknown subcommand '" << tokens[i] << "'.\n";
                }
                return CliResult<void>::Ok();
            }
            target = sub;
        }

        std::cout << HelpFormatter::format_help(*target, target->name());
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
            std::cout << ctx.help_text() << "\n";
            return CliResult<void>::Ok();
        }

        auto *cmd = ctx.matched_command();
        if (!cmd)
            return CliFailure{CliError("no command matched")};
        return cmd->execute(ctx);
    }

}  // namespace pjh::cli
