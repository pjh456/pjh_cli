#include <pjh_cli/converter.hpp>
#include <pjh_cli/detail/string_utils.hpp>
#include <pjh_cli/matcher.hpp>
#include <pjh_cli/parser.hpp>

namespace pjh::cli
{
    namespace
    {
        /// @brief Dispatch a raw string value for a positional arg (no virtual dispatch).
        CliResult<void> apply_arg_value(
            ParseContext &ctx, size_t hash, ValueTag tag, std::string_view s)
        {
            switch (tag)
            {
            case ValueTag::Double:
            {
                auto r = Converter<double>::from_string(s);
                if (r.is_err())
                    return CliResult<void>::Err(std::move(r).unwrap_err());
                ctx.set_value<double>(hash, r.unwrap());
                return CliResult<void>::Ok();
            }
            case ValueTag::String:
                ctx.set_value<std::string>(hash, std::string(s));
                return CliResult<void>::Ok();
            case ValueTag::Path:
                ctx.set_value<std::filesystem::path>(hash, std::filesystem::path(s));
                return CliResult<void>::Ok();
            default:
            {
                auto r = Converter<int>::from_string(s);
                if (r.is_err())
                    return CliResult<void>::Err(std::move(r).unwrap_err());
                ctx.set_value<int>(hash, r.unwrap());
                return CliResult<void>::Ok();
            }
            }
        }

        /// @brief Consume a long option token (--opt or --opt=value).
        ///
        /// Looks up the option on @p cmd. If it expects a value:
        ///   - --opt=val: extracts value from the same token
        ///   - --opt val : advances @p i to consume the next token as value
        /// If it is a flag (bool): sets true in @p ctx.
        /// @param cmd   Current command whose options are consulted.
        /// @param ctx   Parse context to write into.
        /// @param arg   The raw token (e.g. "--port=8080").
        /// @param i     Current index into @p args; advanced when a separate
        ///              value token is consumed.  The caller must still
        ///              increment @p i after the call to skip past the
        ///              option (and its value if consumed).
        /// @param args  Full argument list.
        /// @return Ok on success, or a CliError (unknown option, missing
        ///         value, type conversion failure).
        CliResult<void> consume_long(
            const Command &cmd,
            ParseContext &ctx,
            std::string_view arg,
            size_t &i,
            std::span<const std::string_view> args)
        {
            auto [name, value, has_eq] = detail::split_name_value(arg.substr(2));

            auto *opt = cmd.find_option_by_long(name);
            if (!opt)
            {
                // --no-xxx negation
                if (name.size() > 3 && name.substr(0, 3) == "no-")
                {
                    auto *neg = cmd.find_option_by_long(name.substr(3));
                    if (neg && neg->is_negatable())
                    {
                        ctx.set_value<bool>(neg->key_hash(), false);
                        return CliResult<void>::Ok();
                    }
                }
                return CliFailure{unknown_option(std::format("--{}", name))};
            }

            if (opt->has_value())
            {
                if (has_eq)
                {
                    if (value.empty())
                        return CliFailure{missing_value(std::format("--{}", name))};
                    return opt->parse_value(ctx, value);
                }
                if (++i >= args.size())
                    return CliFailure{missing_value(std::format("--{}", name))};
                return opt->parse_value(ctx, args[i]);
            }

            if (opt->is_counting())
            {
                int cur = ctx.get_value<int>(opt->key_hash(), 0);
                ctx.set_value<int>(opt->key_hash(), cur + 1);
            }
            else
            {
                ctx.set_value<bool>(opt->key_hash(), true);
            }
            return CliResult<void>::Ok();
        }

        /// @brief Consume a short option token (-x, -abc, or -p value).
        ///
        /// Iterates over each character in the token:
        ///   - Bool flags are set to true directly.
        ///   - Valued options must appear as a standalone token (-p value);
        ///     grouped forms like -pvalue or -abp value are rejected.
        ///   When a valued option is found, @p i is advanced to consume the
        ///   next token as its value. The caller must still increment @p i
        ///   after the call.
        /// @param cmd   Current command whose options are consulted.
        /// @param ctx   Parse context to write into.
        /// @param arg   The raw token (e.g. "-abc" or "-p").
        /// @param i     Current index into @p args; advanced when a separate
        ///              value token is consumed.
        /// @param args  Full argument list.
        /// @return Ok on success, or a CliError (unknown option, grouped
        ///         valued option, missing value, type conversion failure).
        CliResult<void> consume_short(
            const Command &cmd,
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
                    return CliFailure{unknown_option(std::format("-{}", c))};

                if (opt->has_value())
                {
                    if (arg.size() != 2)
                        return CliFailure{CliError(
                            std::format(
                                "-{} requires a value as a separate argument", c))};
                    if (++i >= args.size())
                        return CliFailure{missing_value(std::format("-{}", c))};
                    auto r = opt->parse_value(ctx, args[i]);
                    if (r.is_err())
                        return r;
                }
                else if (opt->is_counting())
                {
                    int cur = ctx.get_value<int>(opt->key_hash(), 0);
                    ctx.set_value<int>(opt->key_hash(), cur + 1);
                }
                else
                {
                    ctx.set_value<bool>(opt->key_hash(), true);
                }
            }
            return CliResult<void>::Ok();
        }

        CliResult<ParseContext> finish_parse(
            const Command *cmd, ParseContext ctx, std::string matched_path)
        {
            ctx.set_matched_command(cmd);
            ctx.set_matched_path(std::move(matched_path));

            auto dr = cmd->apply_defaults(ctx);
            if (dr.is_err())
                return CliResult<ParseContext>::Err(std::move(dr).unwrap_err());

            for (const auto &opt_ptr : cmd->options())
            {
                if (opt_ptr->is_required() && !ctx.has_value(opt_ptr->key_hash()))
                {
                    return CliResult<ParseContext>::Err(
                        missing_required_option(opt_ptr->long_name()));
                }
            }

            for (const auto &arg : cmd->args())
            {
                if (arg.m_required && !ctx.has_value(arg.m_key_hash))
                {
                    return CliResult<ParseContext>::Err(missing_required_arg(arg.m_name));
                }
            }

            return CliResult<ParseContext>::Ok(std::move(ctx));
        }

        /// @brief Try to find a subcommand matching @p name on @p cmd.
        ///        Checks exact match first, then fuzzy if @p max_fuzzy > 0.
        /// @return Match result or nullptr.
        ///         On disabled exact match, *out_disabled is set to true.
        const Command *find_subcommand_match(
            const Command &cmd,
            std::string_view name,
            int max_fuzzy_distance,
            bool &out_disabled)
        {
            auto *exact = cmd.find_subcommand(name);
            if (exact)
            {
                if (exact->is_enabled())
                    return exact;
                out_disabled = true;
                return nullptr;
            }

            if (max_fuzzy_distance > 0)
            {
                auto fuzzy = fuzzy_find_subcommands(
                    cmd, name, max_fuzzy_distance, Visibility::Both);
                if (fuzzy.size() == 1)
                    return fuzzy[0].command;
            }

            return nullptr;
        }

    }  // namespace

    CliResult<ParseContext> parse_command(
        const Command &root,
        std::span<const std::string_view> args,
        int max_fuzzy_distance)
    {
        const Command *cmd = &root;
        ParseContext ctx;
        size_t arg_pos = 0;
        bool double_dash = false;

        for (size_t i = 0; i < args.size(); i++)
        {
            auto a = args[i];

            // Handle -- separator
            if (!double_dash && a == "--")
            {
                double_dash = true;
                continue;
            }

            // Handle options for current command
            if (!double_dash && a.size() > 1 && a[0] == '-')
            {
                CliResult<void> r = (a[1] == '-') ? consume_long(*cmd, ctx, a, i, args)
                                                  : consume_short(*cmd, ctx, a, i, args);
                if (r.is_err())
                    return CliResult<ParseContext>::Err(std::move(r).unwrap_err());
                continue;
            }

            // Check for subcommand (only before --)
            if (!double_dash)
            {
                bool disabled = false;
                auto *sub = find_subcommand_match(*cmd, a, max_fuzzy_distance, disabled);

                if (disabled)
                    return CliResult<ParseContext>::Err(command_disabled(a));

                if (sub)
                {
                    cmd = sub;
                    ctx = ParseContext{};
                    arg_pos = 0;
                    continue;
                }
            }

            // Positional/extra arg for current command
            if (arg_pos < cmd->args().size())
            {
                auto &arg = cmd->args()[arg_pos];
                auto r = apply_arg_value(ctx, arg.m_key_hash, arg.m_value_tag, a);
                if (r.is_err())
                    return CliResult<ParseContext>::Err(std::move(r).unwrap_err());
            }
            else
            {
                switch (cmd->extra_args_policy())
                {
                case ExtraArgsPolicy::Error:
                    return CliFailure{parse_error(a, static_cast<int>(i))};
                case ExtraArgsPolicy::Store:
                    ctx.add_extra_arg(std::string(a));
                    break;
                default:
                    break;
                }
            }
            arg_pos++;
        }

        return finish_parse(cmd, std::move(ctx), cmd->name());
    }

}  // namespace pjh::cli
