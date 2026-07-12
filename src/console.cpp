#include <pjh_cli/console.hpp>
#include <pjh_cli/matcher.hpp>
#include <pjh_cli/parser.hpp>

#include <iostream>
#include <sstream>

namespace pjh::cli
{
    InteractiveConsole::InteractiveConsole(
        Command &root,
        std::string prompt)
        : m_root(root),
          m_prompt(std::move(prompt))
    {
    }

    void
    InteractiveConsole::run()
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

            if (line == "quit" ||
                line == "exit" ||
                line == "q")
                break;

            process_line(line);
        }
    }

    void
    InteractiveConsole::stop()
    {
        m_running = false;
    }

    namespace
    {

        std::vector<std::string>
        tokenize_line(
            const std::string &line)
        {
            std::vector<std::string> tokens;
            std::string tok;
            bool in_quote = false;

            for (size_t i = 0; i < line.size(); i++)
            {
                char c = line[i];
                if (c == '"')
                {
                    in_quote = !in_quote;
                    continue;
                }
                if (c == ' ' && !in_quote)
                {
                    if (!tok.empty())
                    {
                        tokens.push_back(std::move(tok));
                        tok.clear();
                    }
                    continue;
                }
                tok += c;
            }
            if (!tok.empty())
                tokens.push_back(std::move(tok));

            return tokens;
        }

    } // namespace

    void
    InteractiveConsole::process_line(
        const std::string &line)
    {
        // "?" or "?query" → search / list
        if (line[0] == '?')
        {
            auto query = line.substr(1);

            if (query.empty())
            {
                auto names = list_subcommands(m_root);
                if (names.empty())
                {
                    std::cout << "No subcommands available.\n";
                }
                else
                {
                    std::cout << "Subcommands:";
                    for (const auto &n : names)
                        std::cout << " " << n;
                    std::cout << "\n";
                }
                return;
            }

            // Search by substring
            bool found = false;
            for (const auto &sub : m_root.subcommands())
            {
                if (!sub.is_enabled())
                    continue;
                if (sub.visibility() == Visibility::Hidden)
                    continue;
                if (sub.name().find(query) != std::string_view::npos)
                {
                    if (!found)
                    {
                        std::cout << "Matching subcommands:";
                        found = true;
                    }
                    std::cout << " " << sub.name();
                }
            }
            if (!found)
            {
                auto fuzzy = fuzzy_find_subcommands(
                    m_root, query, 3);
                if (!fuzzy.empty())
                {
                    std::cout << "Did you mean:";
                    for (const auto &f : fuzzy)
                        std::cout << " " << f.command->name();
                }
                else
                {
                    std::cout << "No matches.";
                    std::cout << " Try: " << format_usage(m_root, m_root.name());
                }
            }
            std::cout << "\n";
            return;
        }

        auto tokens = tokenize_line(line);
        if (tokens.empty())
            return;

        // "help" → show formatted help
        if (tokens.size() == 1 &&
            (tokens[0] == "help" ||
             tokens[0] == "--help" ||
             tokens[0] == "-h"))
        {
            std::cout << format_help(m_root, m_root.name());
            return;
        }

        std::vector<std::string_view> args;
        for (const auto &t : tokens)
            args.emplace_back(t);

        auto r = parse_command_fuzzy(m_root, args);
        if (r.is_err())
        {
            std::cerr << r.unwrap_err().what() << "\n";
            return;
        }

        auto &ctx = r.unwrap();

        // Find the leaf command to execute its action
        const Command *cmd = &m_root;
        for (const auto &t : tokens)
        {
            auto *sub = cmd->find_subcommand(t);
            if (sub && sub->is_enabled())
            {
                cmd = sub;
                continue;
            }
            auto fuzzy = fuzzy_find_subcommands(
                *cmd, t, 3);
            if (fuzzy.size() == 1)
            {
                cmd = fuzzy[0].command;
                continue;
            }
            break;
        }

        auto exec = cmd->execute(ctx);
        if (exec.is_err())
        {
            std::cerr << exec.unwrap_err().what() << "\n";
        }
    }

} // namespace pjh::cli
