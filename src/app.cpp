#include <cstdlib>
#include <iostream>
#include <pjh_cli/app.hpp>
#include <pjh_cli/matcher.hpp>
#include <pjh_cli/parser.hpp>
#include <string>
#include <utility>

#include "pjh_cli/command/branch_command.hpp"
#include "pjh_cli/parse_context.hpp"
#include "pjh_cli/type.hpp"

namespace pjh::cli
{
    namespace
    {
        void print_help_or_exit(BranchCommand &app, int argc, char **argv)
        {
            for (int i = 1; i < argc; i++)
            {
                std::string_view arg = argv[i];
                if (arg != "--help" && arg != "-h")
                    continue;

                BaseCommand *cmd = &app;
                for (int j = 1; j < i; j++)
                {
                    std::string_view tok = argv[j];
                    if (tok.size() > 1 && tok[0] == '-')
                        continue;
                    if (!cmd->is_branch())
                        break;
                    auto *sub = cmd->as_branch()->find_subcommand(tok);
                    if (!sub)
                        continue;
                    cmd = sub;
                }
                std::cout << format_help(*cmd, cmd->name()) << "\n";
                std::exit(0);
            }
        }
    }  // namespace

    App::App(std::string name, std::string version, std::string description) :
        BranchCommand(std::move(name), std::move(description)),
        m_version(std::move(version))
    {
    }

    CliResult<ParseContext> App::parse(int argc, char **argv)
    {
        print_help_or_exit(*this, argc, argv);
        return parse_command(*this, argc, argv);
    }

    CliResult<ParseContext> App::parse_fuzzy(int argc, char **argv)
    {
        print_help_or_exit(*this, argc, argv);
        return parse_command(*this, argc, argv, 3);
    }

}  // namespace pjh::cli
