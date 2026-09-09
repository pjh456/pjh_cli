#include <cctype>
#include <format>
#include <pjh_cli/command/branch_command.hpp>
#include <pjh_cli/core/error.hpp>
#include <pjh_cli/detail/tokenizer.hpp>
#include <pjh_cli/option/option_def.hpp>
#include <pjh_cli/parse/option_consumer.hpp>
#include <pjh_cli/parse/parse_context.hpp>
#include <pjh_cli/parse/parse_context_writer.hpp>

namespace
{
    /// @brief Check if a raw token looks like an option flag (--opt or -v).
    ///        Negative numbers like -5 or -3.14 are NOT detected as flags.
    bool is_option_flag(std::string_view s) noexcept
    {
        return s.size() > 1 && s[0] == '-' &&
               !std::isdigit(static_cast<unsigned char>(s[1])) && s[1] != '.';
    }

    /// @brief True when @p tok exactly names/aliases a direct subcommand of
    ///        @p cmd.  Used to stop greedy repeatable consumption at a command
    ///        boundary so a following subcommand name is not swallowed.
    bool is_subcommand_token(const pjh::cli::BaseCommand &cmd,
                             std::string_view tok) noexcept
    {
        const auto *branch = cmd.as_branch();
        return branch != nullptr && branch->find_subcommand(tok) != nullptr;
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
    /// expects one, and handles --no-xxx negation inline.
    CliResult<void> OptionConsumer::consume_long(
        const BaseCommand &cmd,
        ParseContext &ctx,
        std::string_view arg,
        size_t &i,
        std::span<const std::string_view> args)
    {
        auto parsed = detail::Tokenizer::parse_long_option(arg);

        auto *opt = cmd.find_option_by_long(parsed.name);
        if (!opt)
        {
            if (parsed.is_negation)
            {
                auto *neg = cmd.find_option_by_long(parsed.negated_name);
                if (neg && neg->is_negatable())
                {
                    ParseContextWriter::set_value<bool>(ctx, neg->key_hash(), false);
                    return CliResult<void>::Ok();
                }
            }
            return CliFailure{
                ErrorFactory::unknown_option(std::format("--{}", parsed.name))};
        }

        if (opt->has_value())
        {
            if (parsed.has_equals)
            {
                if (parsed.value.empty())
                    return CliFailure{
                        ErrorFactory::missing_value(std::format("--{}", parsed.name))};
                return opt->parse_value(ctx, parsed.value);
            }
            if (i + 1 >= args.size())
                return CliFailure{
                    ErrorFactory::missing_value(std::format("--{}", parsed.name))};

            auto next = args[i + 1];
            if (is_option_flag(next))
                return CliFailure{
                    ErrorFactory::missing_value(std::format("--{}", parsed.name))};

            auto r = opt->parse_value(ctx, args[++i]);
            if (r.is_err())
                return r;

            while (opt->is_repeatable() && i + 1 < args.size())
            {
                next = args[i + 1];
                if (is_option_flag(next) || is_subcommand_token(cmd, next))
                    break;
                r = opt->parse_value(ctx, args[++i]);
                if (r.is_err())
                    return r;
            }
            return CliResult<void>::Ok();
        }

        apply_flag(opt, ctx);
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
            auto *opt = cmd.find_option_by_short(c);
            if (!opt)
                return CliFailure{ErrorFactory::unknown_option(std::format("-{}", c))};

            if (opt->has_value())
            {
                if (j + 1 < arg.size())
                {
                    auto r = opt->parse_value(ctx, arg.substr(j + 1));
                    if (r.is_err())
                        return r;

                    while (opt->is_repeatable() && i + 1 < args.size())
                    {
                        auto nxt = args[i + 1];
                        if (is_option_flag(nxt) || is_subcommand_token(cmd, nxt))
                            break;
                        r = opt->parse_value(ctx, args[++i]);
                        if (r.is_err())
                            return r;
                    }
                    break;
                }

                if (i + 1 >= args.size())
                    return CliFailure{ErrorFactory::missing_value(std::format("-{}", c))};

                auto next = args[i + 1];
                if (is_option_flag(next))
                    return CliFailure{ErrorFactory::missing_value(std::format("-{}", c))};

                auto r = opt->parse_value(ctx, args[++i]);
                if (r.is_err())
                    return r;

                while (opt->is_repeatable() && i + 1 < args.size())
                {
                    next = args[i + 1];
                    if (is_option_flag(next) || is_subcommand_token(cmd, next))
                        break;
                    r = opt->parse_value(ctx, args[++i]);
                    if (r.is_err())
                        return r;
                }
            }
            else
            {
                apply_flag(opt, ctx);
            }
        }
        return CliResult<void>::Ok();
    }
}  // namespace pjh::cli
