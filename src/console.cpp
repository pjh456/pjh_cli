#include <cstddef>
#include <format>
#include <iostream>
#include <pjh_cli/command/base_command.hpp>
#include <pjh_cli/command/branch_command.hpp>
#include <pjh_cli/console.hpp>
#include <pjh_cli/core/type.hpp>
#include <pjh_cli/detail/tokenizer.hpp>
#include <pjh_cli/format/help_formatter.hpp>
#include <pjh_cli/format/info.hpp>
#include <pjh_cli/format/matcher.hpp>
#include <pjh_cli/parse/parser.hpp>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace pjh::cli
{
    namespace
    {
        std::string format_suggestions(const SuggestionInfo &info)
        {
            std::string out;
            for (auto &m : info.matches)
                out += " " + m.name;
            return out;
        }

        std::string format_no_subcommands()
        {
            return "No subcommands available.";
        }

        std::string format_subcommand_list(const std::vector<std::string> &names)
        {
            if (names.empty())
                return format_no_subcommands();
            std::string out = "Subcommands:";
            for (auto &n : names)
                out += " " + n;
            return out;
        }

        std::string format_matched_subcommands(const std::vector<std::string> &names)
        {
            std::string out = "Matching subcommands:";
            for (auto &n : names)
                out += " " + n;
            return out;
        }

        std::string format_no_match(const std::string &usage)
        {
            return std::format("No matches. Try: {}", usage);
        }

        std::string format_has_no_subcommands(const std::string &name)
        {
            return std::format("'{}' has no subcommands.", name);
        }

        std::string format_unknown_subcommand(
            const std::string &name, std::string_view suggestions)
        {
            if (suggestions.empty())
                return std::format("Unknown subcommand '{}'.", name);
            return std::format(
                "Unknown subcommand '{}'. Did you mean:{}", name, suggestions);
        }
    }

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

    SuggestionInfo InteractiveConsole::collect_fuzzy_suggestions(
        BranchCommand &branch, std::string_view input)
    {
        auto fuzzy = fuzzy_find_subcommands(branch, input, 3, Visibility::Repl);
        SuggestionInfo info;
        info.matches.reserve(fuzzy.size());
        for (auto &f : fuzzy)
            info.matches.push_back({f.command->name(), f.distance});
        return info;
    }

    CliResult<void> InteractiveConsole::handle_query(const std::string &query)
    {
        if (query.empty())
        {
            auto names = list_subcommands(m_root);
            std::cout << format_subcommand_list(names) << "\n";
            return CliResult<void>::Ok();
        }

        std::vector<std::string> matched;
        for (const auto &sub_ptr : m_root.subcommands())
        {
            if (!sub_ptr->is_enabled())
                continue;
            if (sub_ptr->visibility() == Visibility::Hidden)
                continue;
            if (sub_ptr->name().find(query) != std::string_view::npos)
            {
                matched.push_back(sub_ptr->name());
                continue;
            }
            for (const auto &a : sub_ptr->aliases())
                if (a.find(query) != std::string_view::npos)
                {
                    matched.push_back(sub_ptr->name());
                    break;
                }
        }

        if (!matched.empty())
        {
            std::cout << format_matched_subcommands(matched) << "\n";
            return CliResult<void>::Ok();
        }

        auto suggestions = collect_fuzzy_suggestions(m_root, query);
        auto sug_str = format_suggestions(suggestions);
        if (!sug_str.empty())
        {
            std::cout << "Did you mean:" << sug_str << "\n";
        }
        else
        {
            std::cout << format_no_match(
                HelpFormatter::format_usage(m_root, m_root.name()))
                      << "\n";
        }
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
                std::cout << format_has_no_subcommands(target->name()) << "\n";
                return CliResult<void>::Ok();
            }

            auto *branch = target->as_branch();
            auto *sub = branch->find_subcommand(tokens[i]);
            if (!sub)
            {
                auto suggestions = collect_fuzzy_suggestions(*branch, tokens[i]);
                auto sug_str = format_suggestions(suggestions);
                std::cout << format_unknown_subcommand(tokens[i], sug_str) << "\n";
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
            return CliFailure{ErrorFactory::no_command_matched()};
        return cmd->execute(ctx);
    }

}  // namespace pjh::cli
