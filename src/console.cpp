#include <cstddef>
#include <iostream>
#include <pjh_cli/console.hpp>
#include <pjh_cli/detail/tokenizer.hpp>
#include <pjh_cli/matcher.hpp>
#include <pjh_cli/parser.hpp>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "pjh_cli/command/base_command.hpp"
#include "pjh_cli/command/branch_command.hpp"
#include "pjh_cli/type.hpp"

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

    CliResult<void> InteractiveConsole::process_line(const std::string &line)
    {
        if (line.empty())
            return CliResult<void>::Ok();

        // "?" or "?query" → search / list
        if (line[0] == '?')
        {
            auto query = line.substr(1);

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

            // Search by substring
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
                }
            }
            if (!found)
            {
                auto fuzzy = fuzzy_find_subcommands(m_root, query, 3, Visibility::Repl);
                if (!fuzzy.empty())
                {
                    std::cout << "Did you mean:";
                    for (const auto &f : fuzzy) std::cout << " " << f.command->name();
                }
                else
                {
                    std::cout << "No matches.";
                    std::cout << " Try: " << format_usage(m_root, m_root.name());
                }
            }
            std::cout << "\n";
            return CliResult<void>::Ok();
        }

        auto tokens = detail::Tokenizer::tokenize(line);
        if (tokens.empty())
            return CliResult<void>::Ok();

        // "help" or "help <subcommand>" → show formatted help
        if (tokens[0] == "help" || tokens[0] == "--help" || tokens[0] == "-h")
        {
            if (tokens.size() == 1)
            {
                std::cout << format_help(m_root, m_root.name());
            }
            else
            {
                BaseCommand *target = &m_root;
                bool found = true;
                for (size_t i = 1; i < tokens.size(); i++)
                {
                    if (!target->is_branch())
                    {
                        std::cout << "'" << target->name() << "' has no subcommands.\n";
                        found = false;
                        break;
                    }
                    auto *branch = target->as_branch();
                    auto *sub = branch->find_subcommand(tokens[i]);
                    if (!sub)
                    {
                        auto fuzzy = fuzzy_find_subcommands(
                            *branch, tokens[i], 3, Visibility::Repl);
                        if (!fuzzy.empty())
                        {
                            std::cout << "Unknown subcommand '" << tokens[i]
                                      << "'. Did you mean:";
                            for (auto &f : fuzzy) std::cout << " " << f.command->name();
                            std::cout << "\n";
                        }
                        else
                        {
                            std::cout << "Unknown subcommand '" << tokens[i] << "'.\n";
                        }
                        found = false;
                        break;
                    }
                    target = sub;
                }
                if (found)
                    std::cout << format_help(*target, target->name());
            }
            return CliResult<void>::Ok();
        }

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

        return ctx.matched_command()->execute(ctx);
    }

}  // namespace pjh::cli
