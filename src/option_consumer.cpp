#include <format>
#include <pjh_cli/command/branch_command.hpp>
#include <pjh_cli/core/error.hpp>
#include <pjh_cli/detail/string_utils.hpp>
#include <pjh_cli/detail/tokenizer.hpp>
#include <pjh_cli/option/option_def.hpp>
#include <pjh_cli/parse/option_consumer.hpp>
#include <pjh_cli/parse/parse_context.hpp>
#include <pjh_cli/parse/parse_context_writer.hpp>

namespace
{
    /// @brief True when @p tok exactly names/aliases a direct subcommand of
    ///        @p cmd.  Used to stop greedy repeatable consumption at a command
    ///        boundary so a following subcommand name is not swallowed.
    bool is_subcommand_token(const pjh::cli::BaseCommand &cmd,
                             std::string_view tok) noexcept
    {
        const auto *branch = cmd.as_branch();
        return branch != nullptr && branch->find_subcommand(tok) != nullptr;
    }

    /// @brief A resolved option plus how many parent hops away its declaring
    ///        command is (0 == the current command).
    struct OptionMatch
    {
        const pjh::cli::OptionDef *opt = nullptr;
        size_t depth = 0;
    };

    /// @brief Find @p name on @p cmd or its nearest ancestor declaring it.
    OptionMatch find_option_long_in_chain(
        const pjh::cli::BaseCommand &cmd, std::string_view name) noexcept
    {
        size_t depth = 0;
        for (const auto *c = &cmd; c != nullptr; c = c->parent(), ++depth)
            if (const auto *opt = c->find_option_by_long(name))
                return {opt, depth};
        return {};
    }

    /// @brief Find short option @p ch on @p cmd or its nearest ancestor.
    OptionMatch find_option_short_in_chain(
        const pjh::cli::BaseCommand &cmd, char ch) noexcept
    {
        size_t depth = 0;
        for (const auto *c = &cmd; c != nullptr; c = c->parent(), ++depth)
            if (const auto *opt = c->find_option_by_short(ch))
                return {opt, depth};
        return {};
    }

    /// @brief Return the context @p depth parents above @p ctx; @p ctx if the
    ///        chain is shorter than expected (defensive).
    pjh::cli::ParseContext &owner_context(
        pjh::cli::ParseContext &ctx, size_t depth) noexcept
    {
        auto *c = &ctx;
        while (depth > 0 && c != nullptr)
        {
            c = pjh::cli::ParseContextWriter::parent_of(*c);
            --depth;
        }
        return c != nullptr ? *c : ctx;
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
            int cur = ParseContextWriter::get_value<int>(ctx, opt->key_hash(), 0);
            ParseContextWriter::set_value<int>(ctx, opt->key_hash(), cur + 1);
        }
        else
        {
            ParseContextWriter::set_value<bool>(ctx, opt->key_hash(), true);
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

        auto match = find_option_long_in_chain(cmd, parsed.name);
        auto *opt = match.opt;
        if (!opt)
        {
            if (parsed.is_negation)
            {
                auto neg = find_option_long_in_chain(cmd, parsed.negated_name);
                if (neg.opt && neg.opt->is_negatable())
                {
                    if (parsed.has_equals)
                    {
                        return CliFailure{ErrorFactory::option_does_not_accept_value(
                            std::format("--{}", parsed.name))};
                    }
                    ParseContextWriter::set_value<bool>(
                        owner_context(ctx, neg.depth), neg.opt->key_hash(), false);
                    return CliResult<void>::Ok();
                }
            }
            return CliFailure{
                ErrorFactory::unknown_option(std::format("--{}", parsed.name))};
        }
        auto &owner = owner_context(ctx, match.depth);

        if (opt->has_value())
        {
            if (parsed.has_equals)
            {
                if (parsed.value.empty())
                    return CliFailure{
                        ErrorFactory::missing_value(std::format("--{}", parsed.name))};
                return opt->parse_value(owner, parsed.value);
            }
            if (i + 1 >= args.size())
                return CliFailure{
                    ErrorFactory::missing_value(std::format("--{}", parsed.name))};

            auto next = args[i + 1];
            if (detail::is_option_flag(next))
                return CliFailure{
                    ErrorFactory::missing_value(std::format("--{}", parsed.name))};

            auto r = opt->parse_value(owner, args[++i]);
            if (r.is_err())
                return r;

            while (opt->is_repeatable() && i + 1 < args.size())
            {
                next = args[i + 1];
                if (detail::is_option_flag(next) || is_subcommand_token(cmd, next))
                    break;
                r = opt->parse_value(owner, args[++i]);
                if (r.is_err())
                    return r;
            }
            return CliResult<void>::Ok();
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
    /// from the compact form (-p8080) or the next token.
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
            auto match = find_option_short_in_chain(cmd, c);
            auto *opt = match.opt;
            if (!opt)
                return CliFailure{ErrorFactory::unknown_option(std::format("-{}", c))};
            auto &owner = owner_context(ctx, match.depth);

            if (opt->has_value())
            {
                if (j + 1 < arg.size())
                {
                    auto r = opt->parse_value(owner, arg.substr(j + 1));
                    if (r.is_err())
                        return r;

                    while (opt->is_repeatable() && i + 1 < args.size())
                    {
                        auto nxt = args[i + 1];
                        if (detail::is_option_flag(nxt) || is_subcommand_token(cmd, nxt))
                            break;
                        r = opt->parse_value(owner, args[++i]);
                        if (r.is_err())
                            return r;
                    }
                    break;
                }

                if (i + 1 >= args.size())
                    return CliFailure{ErrorFactory::missing_value(std::format("-{}", c))};

                auto next = args[i + 1];
                if (detail::is_option_flag(next))
                    return CliFailure{ErrorFactory::missing_value(std::format("-{}", c))};

                auto r = opt->parse_value(owner, args[++i]);
                if (r.is_err())
                    return r;

                while (opt->is_repeatable() && i + 1 < args.size())
                {
                    next = args[i + 1];
                    if (detail::is_option_flag(next) || is_subcommand_token(cmd, next))
                        break;
                    r = opt->parse_value(owner, args[++i]);
                    if (r.is_err())
                        return r;
                }
            }
            else
            {
                apply_flag(opt, owner);
            }
        }
        return CliResult<void>::Ok();
    }
}  // namespace pjh::cli
