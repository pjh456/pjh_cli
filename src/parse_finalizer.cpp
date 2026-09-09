#include <algorithm>
#include <pjh_cli/command/branch_command.hpp>
#include <pjh_cli/command/leaf_command.hpp>
#include <pjh_cli/core/error.hpp>
#include <pjh_cli/detail/env_snapshot.hpp>
#include <pjh_cli/parse/parse_context_writer.hpp>
#include <pjh_cli/parse/parse_finalizer.hpp>

namespace pjh::cli
{
    /// @brief Apply default values for every option along @p chain.
    ///
    /// Iterates each command in the chain; for each option that has
    /// has_default() true and no user-supplied value, calls
    /// opt->apply_default().
    CliResult<void> ParseFinalizer::apply_chain_defaults(
        const std::vector<BaseCommand *> &chain, ParseContext &ctx)
    {
        for (auto *c : chain)
        {
            auto dr = c->apply_defaults(ctx);
            if (dr.is_err())
                return dr;
        }
        return CliResult<void>::Ok();
    }

    /// @brief Fall back to environment variables for unset options.
    ///
    /// Reads the env snapshot from the root command.  For each option that
    /// has a non-empty env_var() and no value set, looks up the env var
    /// and calls opt->parse_value().
    CliResult<void> ParseFinalizer::apply_chain_env(
        const std::vector<BaseCommand *> &chain, ParseContext &ctx)
    {
        auto *env_snap = chain[0]->env_snapshot();
        if (!env_snap)
            return CliResult<void>::Ok();

        for (auto *c : chain)
        {
            for (const auto &opt_ptr : c->options())
            {
                if (!ParseContextWriter::has_value(ctx, opt_ptr->key_hash()) &&
                    !opt_ptr->env_var().empty())
                {
                    auto *env_val = env_snap->get(opt_ptr->env_var());
                    if (env_val)
                    {
                        auto r = opt_ptr->parse_value(ctx, *env_val);
                        if (r.is_err())
                            return r;
                    }
                }
            }
        }
        return CliResult<void>::Ok();
    }

    /// @brief Check that every required option has been set.
    ///
    /// If any option with is_required() true is still absent from @p ctx,
    /// returns a missing_required_option error.
    CliResult<void> ParseFinalizer::check_required_options(
        const std::vector<BaseCommand *> &chain, ParseContext &ctx)
    {
        for (auto *c : chain)
        {
            for (const auto &opt_ptr : c->options())
            {
                if (opt_ptr->is_required() &&
                    !ParseContextWriter::has_value(ctx, opt_ptr->key_hash()))
                    return CliFailure{
                        ErrorFactory::missing_required_option(opt_ptr->long_name())};
            }
        }
        return CliResult<void>::Ok();
    }

    /// @brief Validate option group constraints.
    ///
    /// For each registered group on each command in the chain, checks
    /// that the constraint (ExactlyOne / AtMostOne / AtLeastOne) is
    /// satisfied.
    CliResult<void> ParseFinalizer::validate_groups(
        const std::vector<BaseCommand *> &chain, ParseContext &ctx)
    {
        for (auto *c : chain)
        {
            for (auto &group : c->groups())
            {
                size_t count = 0;
                for (auto h : group.key_hashes)
                    if (ParseContextWriter::has_value(ctx, h))
                        count++;

                switch (group.mode)
                {
                case GroupMode::ExactlyOne:
                    if (count == 0)
                        return CliFailure{ErrorFactory::required_option_group(
                            group.option_names, true)};
                    if (count > 1)
                        return CliFailure{
                            ErrorFactory::conflicting_options(group.option_names)};
                    break;
                case GroupMode::AtMostOne:
                    if (count > 1)
                        return CliFailure{
                            ErrorFactory::conflicting_options(group.option_names)};
                    break;
                case GroupMode::AtLeastOne:
                    if (count == 0)
                        return CliFailure{ErrorFactory::required_option_group(
                            group.option_names, false)};
                    break;
                }
            }
        }
        return CliResult<void>::Ok();
    }

    /// @brief Check required positional arguments on the leaf command.
    ///
    /// If @p cmd is a leaf, iterates its positional args and returns
    /// a missing_required_arg error for any that are required but absent.
    CliResult<void> ParseFinalizer::check_required_args(
        BaseCommand *cmd, ParseContext &ctx)
    {
        if (auto *leaf = cmd->as_leaf())
        {
            for (const auto &arg : leaf->args())
            {
                if (arg.m_required && !ParseContextWriter::has_value(ctx, arg.m_key_hash))
                    return CliFailure{ErrorFactory::missing_required_arg(arg.m_name)};
            }
        }
        return CliResult<void>::Ok();
    }

    /// @brief Finalise a parse result by applying env-vars, defaults,
    ///        required checks, and group validation.
    ///
    /// Chains together the five validation steps and returns the final
    /// ParseContext on success, or the first error encountered.  Values
    /// resolve with CLI > env > default precedence: env fills unset options
    /// first, then defaults fill whatever env did not.
    CliResult<ParseContext> ParseFinalizer::finalize(BaseCommand *cmd, ParseContext ctx)
    {
        ParseContextWriter::set_matched_command(ctx, cmd);

        std::vector<BaseCommand *> chain;
        for (auto *c = cmd; c; c = c->parent()) chain.push_back(c);
        std::reverse(chain.begin(), chain.end());

        auto er = apply_chain_env(chain, ctx);
        if (er.is_err())
            return CliResult<ParseContext>::Err(std::move(er).unwrap_err());

        auto dr = apply_chain_defaults(chain, ctx);
        if (dr.is_err())
            return CliResult<ParseContext>::Err(std::move(dr).unwrap_err());

        auto rr = check_required_options(chain, ctx);
        if (rr.is_err())
            return CliResult<ParseContext>::Err(std::move(rr).unwrap_err());

        auto gr = validate_groups(chain, ctx);
        if (gr.is_err())
            return CliResult<ParseContext>::Err(std::move(gr).unwrap_err());

        auto ar = check_required_args(cmd, ctx);
        if (ar.is_err())
            return CliResult<ParseContext>::Err(std::move(ar).unwrap_err());

        return CliResult<ParseContext>::Ok(std::move(ctx));
    }
}  // namespace pjh::cli
