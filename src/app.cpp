#include <pjh_cli/app.hpp>
#include <pjh_cli/command/branch_command.hpp>
#include <pjh_cli/core/type.hpp>
#include <pjh_cli/parse/parse_context.hpp>
#include <pjh_cli/parse/parser.hpp>
#include <string>
#include <utility>

namespace pjh::cli
{

    App::App(std::string name, std::string version, std::string description) :
        BranchCommand(std::move(name), std::move(description)),
        m_version(std::move(version))
    {
    }

    CliResult<ParseContext> App::parse(int argc, char **argv)
    {
        return Parser::parse_command(*this, argc, argv);
    }

    CliResult<ParseContext> App::parse_fuzzy(int argc, char **argv)
    {
        return Parser::parse_command(*this, argc, argv, 3);
    }

}  // namespace pjh::cli
