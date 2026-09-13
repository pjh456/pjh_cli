#include <algorithm>
#include <cstddef>
#include <format>
#include <pjh_cli/command/arg_scan.hpp>
#include <pjh_cli/command/branch_command.hpp>
#include <pjh_cli/command/matcher.hpp>
#include <pjh_cli/core/error.hpp>
#include <pjh_cli/detail/string_utils.hpp>
#include <pjh_cli/detail/tokenizer.hpp>
#include <pjh_cli/option/option_def.hpp>
#include <pjh_cli/parse/detail/parse_context_writer.hpp>
#include <pjh_cli/parse/option_consumer.hpp>
#include <pjh_cli/parse/parse_context.hpp>
#include <pjh_cli/parse/value_writer.hpp>
#include <unordered_set>
#include <utility>
#include <vector>

namespace
{
    /// @brief Maximum Levenshtein distance for a long-option suggestion.
    ///
    /// 2 (not the subcommand precedent's 3): option names are short, so a
    /// distance-3 cutoff would suggest --verbose for the non-negatable
    /// --no-verbose token (distance 3), where the named option exists but
    /// simply does not support negation.
    constexpr int k_suggestion_distance = 2;

    /// @brief Maximum number of rendered long-option suggestions.
    constexpr std::size_t k_max_suggestions = 3;

    /// @brief Long-option displays near @p name on @p cmd or its ancestors.
    ///
    /// Walks the same current→ancestor chain as
    /// find_option_by_long_in_chain_with_depth(),
    /// keeps options whose declaring command is visible+enabled, and returns
    /// "--name" displays within k_suggestion_distance, closest first, capped at
    /// k_max_suggestions.  Options have no aliases, so one candidate per
    /// declared long name; the nearest declaration wins on duplicates.
    /// Candidates whose long-name length differs from @p name by more than
    /// k_suggestion_distance are rejected without running the distance
    /// (necessary condition, results unchanged).
    ///
    /// @param cmd   Command in scope at the miss.
    /// @param name  Typed long-option name without the leading dashes.
    /// @return Display strings ("--port"), possibly empty.
    /// @throws std::bad_alloc if a candidate list cannot be allocated.
    /// @throws Any exception thrown by a user enabled predicate
    ///         (is_visible_and_enabled is not noexcept).
    std::vector<std::string> suggest_long_options(
        const pjh::cli::BaseCommand &cmd, std::string_view name)
    {
        // Chain-wide upper bound (also counts hidden/disabled commands, so it
        // only over-reserves): keeps the per-call containers from growing.
        std::size_t upper = 0;
        for (const auto *c = &cmd; c != nullptr; c = c->parent())
            upper += c->options().size();

        std::vector<std::pair<std::string, int>> matches;
        std::unordered_set<std::string_view> seen;
        matches.reserve(upper);
        seen.reserve(upper);
        for (const auto *c = &cmd; c != nullptr; c = c->parent())
        {
            if (!pjh::cli::detail::is_visible_and_enabled(*c, pjh::cli::Visibility::Both))
                continue;
            for (const auto &opt : c->options())
            {
                if (opt->long_name().empty())
                    continue;  // unreachable via long lookup; nothing to suggest.
                if (!seen.insert(opt->long_name()).second)
                    continue;  // nearest declaration wins.
                if (!pjh::cli::detail::within_edit_distance_bound(
                        name, opt->long_name(), k_suggestion_distance))
                    continue;  // provably farther than k_suggestion_distance.
                const int d = pjh::cli::edit_distance(name, opt->long_name());
                if (d <= k_suggestion_distance)
                    matches.emplace_back(opt->display_name(), d);
            }
        }
        std::ranges::stable_sort(matches, {}, [](const auto &m) { return m.second; });
        std::vector<std::string> out;
        out.reserve(k_max_suggestions);
        for (auto &m : matches)
        {
            if (out.size() == k_max_suggestions)
                break;
            out.push_back(std::move(m.first));
        }
        return out;
    }

    /// @brief Return the context @p depth parents above @p ctx; @p ctx if the
    ///        chain is shorter than expected (defensive).
    pjh::cli::ParseContext &owner_context(
        pjh::cli::ParseContext &ctx, size_t depth) noexcept
    {
        auto *c = &ctx;
        while (depth > 0 && c != nullptr)
        {
            c = pjh::cli::detail::ParseContextWriter::parent_of(*c);
            --depth;
        }
        return c != nullptr ? *c : ctx;
    }

    /// @brief Greedily consume following tokens as values of a repeatable option.
    ///
    /// Stops at end of input, at an option-looking token (is_option_flag), or at
    /// a direct subcommand name of @p cmd (so a following subcommand is not
    /// swallowed).  Writes each consumed value via ValueWriter.
    ///
    /// @param cmd    Command in scope, used for the subcommand stop predicate.
    /// @param owner  Context of the option's declaring command.
    /// @param opt    The repeatable option being filled.
    /// @param i      Index of the last consumed token; advanced per value.
    /// @param args   Full argument list.
    /// @return Ok after the run stops; the first conversion failure otherwise.
    pjh::cli::CliResult<void> consume_repeatable(
        const pjh::cli::BaseCommand &cmd,
        pjh::cli::ParseContext &owner,
        const pjh::cli::OptionDef &opt,
        size_t &i,
        std::span<const std::string_view> args)
    {
        while (opt.is_repeatable() && i + 1 < args.size())
        {
            auto next = args[i + 1];
            if (pjh::cli::detail::is_option_flag(next) ||
                pjh::cli::detail::is_subcommand_of(cmd, next))
                break;
            auto r = pjh::cli::ValueWriter::apply_option_raw(owner, opt, args[++i]);
            if (r.is_err())
                return r;
        }
        return pjh::cli::CliResult<void>::Ok();
    }
}

namespace pjh::cli
{
    /// @brief Set a flag or increment a counter on @p ctx.
    ///
    /// If the option is a CountingOption, the stored int is incremented;
    /// otherwise the option is treated as a boolean flag and set to true.
    void OptionConsumer::apply_flag(const OptionDef *opt, ParseContext &ctx)
    {
        if (opt->is_counting())
        {
            int cur = detail::ParseContextWriter::get_value<int>(ctx, opt->key_hash(), 0);
            detail::ParseContextWriter::set_value<int>(ctx, opt->key_hash(), cur + 1);
        }
        else
        {
            detail::ParseContextWriter::set_value<bool>(ctx, opt->key_hash(), true);
        }
    }

    /// @brief Consume a single long-option token (--opt or --opt=val).
    ///
    /// Parses the token via Tokenizer::parse_long_option(), looks up the
    /// option on @p cmd, extracts and converts the value when the option
    /// expects one, and handles --no-xxx negation inline.  A negated flag
    /// (--no-xxx) takes no value: --no-xxx=v is rejected with
    /// option_does_not_accept_value.
    CliResult<void> OptionConsumer::consume_long(
        const BaseCommand &cmd,
        ParseContext &ctx,
        std::string_view arg,
        size_t &i,
        std::span<const std::string_view> args)
    {
        auto parsed = detail::Tokenizer::parse_long_option(arg);

        auto match = detail::find_option_by_long_in_chain_with_depth(cmd, parsed.name);
        auto *opt = match.option;
        if (!opt)
        {
            if (parsed.is_negation)
            {
                auto neg = detail::find_option_by_long_in_chain_with_depth(
                    cmd, parsed.negated_name);
                if (neg.option && neg.option->is_negatable())
                {
                    if (parsed.has_equals)
                    {
                        return CliFailure{ErrorFactory::option_does_not_accept_value(
                            std::format("--{}", parsed.name))};
                    }
                    detail::ParseContextWriter::set_value<bool>(
                        owner_context(ctx, neg.depth), neg.option->key_hash(), false);
                    return CliResult<void>::Ok();
                }
            }
            return CliFailure{ErrorFactory::unknown_option(
                std::format("--{}", parsed.name),
                suggest_long_options(cmd, parsed.name))};
        }
        auto &owner = owner_context(ctx, match.depth);

        if (opt->has_value())
        {
            if (parsed.has_equals)
            {
                if (parsed.value.empty())
                    return CliFailure{
                        ErrorFactory::missing_value(std::format("--{}", parsed.name))};
                return ValueWriter::apply_option_raw(owner, *opt, parsed.value);
            }
            if (i + 1 >= args.size())
                return CliFailure{
                    ErrorFactory::missing_value(std::format("--{}", parsed.name))};

            auto next = args[i + 1];
            if (detail::is_option_flag(next))
                return CliFailure{
                    ErrorFactory::missing_value(std::format("--{}", parsed.name))};

            auto r = ValueWriter::apply_option_raw(owner, *opt, args[++i]);
            if (r.is_err())
                return r;

            return consume_repeatable(cmd, owner, *opt, i, args);
        }

        if (parsed.has_equals)
        {
            return CliFailure{ErrorFactory::option_does_not_accept_value(
                std::format("--{}", parsed.name))};
        }

        apply_flag(opt, owner);
        return CliResult<void>::Ok();
    }

    /// @brief Consume a short-option token (-x, -abc, or -p value).
    ///
    /// Iterates each character in the token.  Bool flags are set directly,
    /// counting options are incremented, and valued options consume a value
    /// from the compact form (-p8080 or -p=8080) or the next token.  Exactly
    /// one leading '=' is stripped from the compact remainder, so -p=8080
    /// equals -p8080 and -p==x yields the literal "=x"; an empty compact value
    /// (-p=) is a missing value.  A flag/count option followed by '=' (-v=1)
    /// is rejected with option_does_not_accept_value, mirroring the long form.
    CliResult<void> OptionConsumer::consume_short(
        const BaseCommand &cmd,
        ParseContext &ctx,
        std::string_view arg,
        size_t &i,
        std::span<const std::string_view> args)
    {
        for (size_t j = 1; j < arg.size(); j++)
        {
            char c = arg[j];
            auto match = detail::find_option_by_short_in_chain_with_depth(cmd, c);
            auto *opt = match.option;
            if (!opt)
                return CliFailure{ErrorFactory::unknown_option(std::format("-{}", c))};
            auto &owner = owner_context(ctx, match.depth);

            if (opt->has_value())
            {
                if (j + 1 < arg.size())
                {
                    auto value = arg.substr(j + 1);
                    if (value.front() == '=')
                    {
                        value.remove_prefix(1);
                        if (value.empty())
                            return CliFailure{
                                ErrorFactory::missing_value(std::format("-{}", c))};
                    }

                    auto r = ValueWriter::apply_option_raw(owner, *opt, value);
                    if (r.is_err())
                        return r;

                    return consume_repeatable(cmd, owner, *opt, i, args);
                }

                if (i + 1 >= args.size())
                    return CliFailure{ErrorFactory::missing_value(std::format("-{}", c))};

                auto next = args[i + 1];
                if (detail::is_option_flag(next))
                    return CliFailure{ErrorFactory::missing_value(std::format("-{}", c))};

                auto r = ValueWriter::apply_option_raw(owner, *opt, args[++i]);
                if (r.is_err())
                    return r;

                return consume_repeatable(cmd, owner, *opt, i, args);
            }
            else
            {
                if (j + 1 < arg.size() && arg[j + 1] == '=')
                    return CliFailure{ErrorFactory::option_does_not_accept_value(
                        std::format("-{}", c))};
                apply_flag(opt, owner);
            }
        }
        return CliResult<void>::Ok();
    }
}  // namespace pjh::cli
