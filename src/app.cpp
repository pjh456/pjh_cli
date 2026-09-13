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

    /// @brief Build a structured failure result from a CliError.
    pjh::cli::AppRunResult error_result(pjh::cli::CliError error)
    {
        pjh::cli::AppRunResult result;
        result.kind = error.kind() == pjh::cli::ErrorKind::Parse
                          ? pjh::cli::AppRunResult::Kind::ParseError
                          : pjh::cli::AppRunResult::Kind::RuntimeError;
        result.error = pjh::result::Option<pjh::cli::CliError>::Some(std::move(error));
        return result;
    }

    /// @brief Non-printing dispatch tail of App::run_quiet() / run_fuzzy_quiet().
    ///
    /// Moves help/version text into the result or executes the matched action,
    /// classifying any failure; it never touches a stream.
    pjh::cli::AppRunResult dispatch_quiet(pjh::cli::ParseContext &ctx)
    {
        if (ctx.help_requested())
        {
            pjh::cli::AppRunResult result;
            result.kind = pjh::cli::AppRunResult::Kind::Help;
            result.text = ctx.help_text();
            return result;
        }
        if (ctx.version_requested())
        {
            pjh::cli::AppRunResult result;
            result.kind = pjh::cli::AppRunResult::Kind::Version;
            result.text = ctx.version_text();
            return result;
        }

        auto *cmd = ctx.matched_command();
        if (!cmd)
        {
            pjh::cli::AppRunResult result;
            result.kind = pjh::cli::AppRunResult::Kind::NoCommand;
            result.error = pjh::result::Option<pjh::cli::CliError>::Some(
                pjh::cli::ErrorFactory::no_command_matched());
            return result;
        }

        auto executed = cmd->execute(ctx);
        if (executed.is_err())
        {
            return error_result(executed.unwrap_err());
        }
        return pjh::cli::AppRunResult{};
    }

    /// @brief Render a structured result through the legacy run() streams.
    int render_result(
        const pjh::cli::AppRunResult &result, std::ostream &out, std::ostream &err)
    {
        if (result.kind == pjh::cli::AppRunResult::Kind::Help ||
            result.kind == pjh::cli::AppRunResult::Kind::Version)
        {
            out << result.text;
            return pjh::cli::kExitSuccess;
        }
        if (result.error.is_some())
        {
            err << result.error.unwrap().what() << "\n";
        }
        return result.exit_code();
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

    int AppRunResult::exit_code() const noexcept
    {
        if (error.is_none())
        {
            return kExitSuccess;
        }
        return exit_code_for(error.unwrap());
    }

    int App::run(int argc, char **argv) { return run(argc, argv, std::cout, std::cerr); }

    int App::run(int argc, char **argv, std::ostream &out, std::ostream &err)
    {
        return render_result(run_quiet(argc, argv), out, err);
    }

    int App::run_fuzzy(int argc, char **argv)
    {
        return run_fuzzy(argc, argv, std::cout, std::cerr);
    }

    int App::run_fuzzy(int argc, char **argv, std::ostream &out, std::ostream &err)
    {
        return render_result(run_fuzzy_quiet(argc, argv), out, err);
    }

    AppRunResult App::run_quiet(int argc, char **argv)
    {
        auto parsed = parse(argc, argv);
        if (parsed.is_err())
        {
            return error_result(parsed.unwrap_err());
        }
        return dispatch_quiet(parsed.unwrap());
    }

    AppRunResult App::run_fuzzy_quiet(int argc, char **argv)
    {
        auto parsed = parse_fuzzy(argc, argv);
        if (parsed.is_err())
        {
            return error_result(parsed.unwrap_err());
        }
        return dispatch_quiet(parsed.unwrap());
    }

}  // namespace pjh::cli
