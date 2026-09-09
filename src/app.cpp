#include <iostream>
#include <pjh_cli/app.hpp>
#include <pjh_cli/command/branch_command.hpp>
#include <pjh_cli/core/error.hpp>
#include <pjh_cli/core/type.hpp>
#include <pjh_cli/parse/parse_context.hpp>
#include <pjh_cli/parse/parser.hpp>
#include <string>
#include <utility>

namespace
{
    /// @brief Map a CliError to the App::run() exit-code contract.
    int exit_code_for(const pjh::cli::CliError &error)
    {
        return error.kind() == pjh::cli::ErrorKind::Parse ? pjh::cli::kExitParseError
                                                          : pjh::cli::kExitRuntimeError;
    }

    /// @brief Shared dispatch tail of App::run() / App::run_fuzzy().
    ///
    /// Prints help/version to @p out or executes the matched action,
    /// printing any error to @p err, and returns the exit code.
    int dispatch(pjh::cli::ParseContext &ctx, std::ostream &out, std::ostream &err)
    {
        if (ctx.help_requested())
        {
            out << ctx.help_text();
            return pjh::cli::kExitSuccess;
        }
        if (ctx.version_requested())
        {
            out << ctx.version_text();
            return pjh::cli::kExitSuccess;
        }

        auto *cmd = ctx.matched_command();
        if (!cmd)
        {
            err << pjh::cli::ErrorFactory::no_command_matched().what() << "\n";
            return pjh::cli::kExitParseError;
        }

        auto executed = cmd->execute(ctx);
        if (executed.is_err())
        {
            auto error = executed.unwrap_err();
            err << error.what() << "\n";
            return exit_code_for(error);
        }
        return pjh::cli::kExitSuccess;
    }
}  // namespace

namespace pjh::cli
{

    App::App(std::string name, std::string version, std::string description) :
        BranchCommand(std::move(name), std::move(description)),
        m_version(std::move(version))
    {
    }

    CliResult<ParseContext> App::parse(int argc, char **argv)
    {
        return Parser::parse_command(*this, argc, argv, 0, m_help_formatter);
    }

    CliResult<ParseContext> App::parse_fuzzy(int argc, char **argv)
    {
        return Parser::parse_command(*this, argc, argv, 3, m_help_formatter);
    }

    int App::run(int argc, char **argv) { return run(argc, argv, std::cout, std::cerr); }

    int App::run(int argc, char **argv, std::ostream &out, std::ostream &err)
    {
        auto parsed = parse(argc, argv);
        if (parsed.is_err())
        {
            auto error = parsed.unwrap_err();
            err << error.what() << "\n";
            return exit_code_for(error);
        }
        return dispatch(parsed.unwrap(), out, err);
    }

    int App::run_fuzzy(int argc, char **argv)
    {
        return run_fuzzy(argc, argv, std::cout, std::cerr);
    }

    int App::run_fuzzy(int argc, char **argv, std::ostream &out, std::ostream &err)
    {
        auto parsed = parse_fuzzy(argc, argv);
        if (parsed.is_err())
        {
            auto error = parsed.unwrap_err();
            err << error.what() << "\n";
            return exit_code_for(error);
        }
        return dispatch(parsed.unwrap(), out, err);
    }

}  // namespace pjh::cli
