#include <format>
#include <pjh_cli/core/error.hpp>
#include <pjh_cli/detail/env_snapshot.hpp>
#include <pjh_cli/format/help_formatter.hpp>
#include <pjh_cli/format/info.hpp>
#include <pjh_cli/parse/option_consumer.hpp>
#include <pjh_cli/parse/parse_context.hpp>
#include <pjh_cli/parse/parse_context_writer.hpp>
#include <pjh_cli/parse/parse_finalizer.hpp>
#include <pjh_cli/parse/parser.hpp>
#include <pjh_cli/parse/subcommand_resolver.hpp>
#include <pjh_cli/parse/value_writer.hpp>

namespace pjh::cli
{
    /// @brief If the current token is --help or -h, return a help-only
    ///        ParseContext immediately.
    ///
    /// The returned context has help_requested() == true.  The caller
    /// should check this flag and print help_text() instead of executing
    /// the command action.
    pjh::result::Option<ParseContext> Parser::try_handle_help(
        BaseCommand *cmd, ParseContext &&ctx, std::string_view a, bool double_dash)
    {
        if (double_dash || (a != "--help" && a != "-h"))
            return pjh::result::Option<ParseContext>::None();

        ParseContextWriter::set_help_text(ctx, HelpFormatter::format_help(*cmd));
        ParseContextWriter::set_matched_command(ctx, cmd);
        return pjh::result::Option<ParseContext>::Some(std::move(ctx));
    }

    /// @brief If the current token is --version, return a version-only
    ///        ParseContext immediately.
    ///
    /// The returned context has version_requested() == true.  The caller
    /// should print version_text() instead of executing the action.
    pjh::result::Option<ParseContext> Parser::try_handle_version(
        BaseCommand &root, ParseContext &&ctx, std::string_view a, bool double_dash)
    {
        if (double_dash || a != "--version")
            return pjh::result::Option<ParseContext>::None();

        VersionInfo vi{std::string(root.name()), root.version()};
        ParseContextWriter::set_version_text(ctx,
            std::format("{} version {}\n", vi.program_name, vi.version));
        ParseContextWriter::set_matched_command(ctx, &root);
        return pjh::result::Option<ParseContext>::Some(std::move(ctx));
    }

    /// @brief Handle an unrecognised token per ExtraArgsPolicy.
    ///
    /// On a branch that has subcommands, the implicit `Ignore` default is
    /// overridden to an unknown-command error (with fuzzy suggestions) unless
    /// the policy was set explicitly or a `--` barrier is active.  Otherwise:
    /// Error returns a parse_error, Store appends to extra_args(), Ignore
    /// is a no-op.
    CliResult<void> Parser::handle_extra_arg(
        BaseCommand *cmd,
        ParseContext &ctx,
        std::string_view a,
        size_t pos,
        bool double_dash)
    {
        if (!double_dash && cmd->is_branch() && !cmd->extra_args_explicit() &&
            cmd->extra_args_policy() == ExtraArgsPolicy::Ignore &&
            !cmd->as_branch()->subcommands().empty())
        {
            return CliFailure{
                SubcommandResolver::unknown_subcommand(*cmd->as_branch(), a)};
        }

        switch (cmd->extra_args_policy())
        {
        case ExtraArgsPolicy::Error:
            return CliFailure{ErrorFactory::parse_error(a, static_cast<int>(pos))};
        case ExtraArgsPolicy::Store:
            ParseContextWriter::add_extra_arg(ctx, std::string(a));
            break;
        default:
            break;
        }
        return CliResult<void>::Ok();
    }

    /// @brief Parse a span of string_views against the command tree.
    ///
    /// Walks @p args in a single pass:
    ///   1. `--`               → double-dash terminator
    ///   2. `--help` / `-h`    → return help-only context immediately
    ///   3. `--version`        → return version-only context immediately
    ///   4. `--opt` / `-x`     → delegate to OptionConsumer
    ///   5. word token         → try SubcommandResolver descent,
    ///                           then positional arg via ValueWriter,
    ///                           then ExtraArgsPolicy dispatch
    ///   6. end of args        → ParseFinalizer::finalize
    CliResult<ParseContext> Parser::parse_command(
        BaseCommand &root,
        std::span<const std::string_view> args,
        int max_fuzzy_distance)
    {
        BaseCommand *cmd = &root;
        ParseContext ctx;
        size_t arg_pos = 0;
        bool double_dash = false;

        for (size_t i = 0; i < args.size(); i++)
        {
            auto a = args[i];

            if (!double_dash && a == "--")
            {
                double_dash = true;
                continue;
            }

            {
                auto help = try_handle_help(cmd, std::move(ctx), a, double_dash);
                if (help.is_some())
                    return CliResult<ParseContext>::Ok(std::move(help).unwrap());
            }

            {
                auto ver = try_handle_version(root, std::move(ctx), a, double_dash);
                if (ver.is_some())
                    return CliResult<ParseContext>::Ok(std::move(ver).unwrap());
            }

            if (!double_dash && a.size() > 1 && a[0] == '-')
            {
                CliResult<void> r = (a[1] == '-')
                                        ? OptionConsumer::consume_long(
                                              *cmd, ctx, a, i, args)
                                        : OptionConsumer::consume_short(
                                              *cmd, ctx, a, i, args);
                if (r.is_err())
                    return CliResult<ParseContext>::Err(std::move(r).unwrap_err());
                continue;
            }

            {
                auto sr = SubcommandResolver::try_descend_subcommand(
                    cmd, ctx, a, max_fuzzy_distance, double_dash);
                if (sr.is_err())
                    return CliResult<ParseContext>::Err(std::move(sr).unwrap_err());
                if (sr.unwrap().matched)
                {
                    auto &r = sr.unwrap();
                    cmd = r.cmd;
                    ctx = std::move(r.ctx);
                    arg_pos = 0;
                    continue;
                }
            }

            if (auto *leaf = cmd->as_leaf();
                leaf && arg_pos < leaf->args().size())
            {
                auto &arg = leaf->args()[arg_pos];
                auto r = ValueWriter::apply_arg_value(
                    ctx, arg.m_key_hash, arg.m_value_tag, a, arg.m_name);
                if (r.is_err())
                    return CliResult<ParseContext>::Err(std::move(r).unwrap_err());
            }
            else
            {
                auto r = handle_extra_arg(cmd, ctx, a, i, double_dash);
                if (r.is_err())
                    return CliResult<ParseContext>::Err(std::move(r).unwrap_err());
            }
            arg_pos++;
        }

        return ParseFinalizer::finalize(cmd, std::move(ctx));
    }

    /// @brief Convenience: converts argv[1..argc-1] to a span and delegates
    ///        to the span overload.
    CliResult<ParseContext> Parser::parse_command(
        BaseCommand &root, int argc, char **argv, int max_fuzzy_distance)
    {
        std::vector<std::string_view> args;
        args.reserve(static_cast<size_t>(argc) - 1);
        for (int a = 1; a < argc; a++) args.emplace_back(argv[a]);
        return parse_command(root, args, max_fuzzy_distance);
    }
}  // namespace pjh::cli
