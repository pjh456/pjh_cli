#ifndef INCLUDE_PJH_CLI_PARSE_OPTION_CONSUMER_HPP
#define INCLUDE_PJH_CLI_PARSE_OPTION_CONSUMER_HPP

#include <pjh_cli/command/base_command.hpp>
#include <pjh_cli/core/type.hpp>
#include <pjh_cli/parse/parse_context.hpp>
#include <span>
#include <string_view>

namespace pjh::cli
{
    /// @brief Consumes option tokens (--opt / -x) from the argument list.
    ///
    /// Looks up option definitions on the current command or its nearest
    /// ancestor declaring the option (nearest declaration wins); siblings and
    /// descendants are never consulted.  Extracts values (from = syntax, next
    /// token, or compact form), handles negation (--no-xxx), and writes the
    /// parsed value into the declaring command's ParseContext so repeatable and
    /// counting ancestor options accumulate regardless of where the token
    /// appears.
    ///
    /// Both consume_long() and consume_short() accept a mutable index @p i
    /// that is advanced when a separate value token is consumed, so the
    /// caller's main loop can skip past it naturally.  The value/greedy guards
    /// use the shared detail::is_option_flag predicate, so negative numbers
    /// such as -5 and -3.14 are accepted as values, never as flags.
    class OptionConsumer
    {
    public:
        OptionConsumer() = delete;

        /// @brief Consume a single long-option token (--opt or --opt=val).
        ///
        /// Tokenizes through detail::Tokenizer::parse_long_option() to split
        /// the name, value, and negation state.  Looks up the option on @p cmd
        /// or its nearest ancestor declaring it; the value is stored in the
        /// declaring command's context.
        /// If the option expects a value:
        ///   - =value form: value is taken from after the '='
        ///   - next-token form: the next argument is consumed via @p i
        /// If the token is --no-<name> and the option is negatable, sets false;
        /// a =value on the negated form is rejected
        /// (option_does_not_accept_value).
        /// For flag/count options, apply_flag() is called.
        /// For repeatable options, greedily consumes following non-flag tokens.
        ///
        /// @param cmd   Current command whose options and ancestors are consulted.
        /// @param ctx   Parse context to write into (owner context is derived).
        /// @param arg   The raw token (e.g. "--port=8080").
        /// @param i     Current index into @p args; advanced when a separate
        ///              value token is consumed.
        /// @param args  Full argument list.
        /// @return Ok on success, or an appropriate CliError.
        static CliResult<void> consume_long(
            const BaseCommand &cmd,
            ParseContext &ctx,
            std::string_view arg,
            size_t &i,
            std::span<const std::string_view> args);

        /// @brief Consume a short-option token (-x, -abc, or -p value).
        ///
        /// Iterates over each character in the token:
        ///   - Bool flags are set to true directly.
        ///   - Counting options are incremented.
        ///   - Valued options support compact form (-p8080) or separated form
        ///     (-p 8080); compact form must have the value immediately after
        ///     the option character.  Grouped short options like -vp where
        ///     'p' expects a value consume the next token.
        /// For repeatable valued options, greedily consumes following
        /// non-flag tokens after the value.
        ///
        /// @param cmd   Current command whose options and ancestors are consulted.
        /// @param ctx   Parse context to write into (owner context is derived).
        /// @param arg   The raw token (e.g. "-abc" or "-p").
        /// @param i     Current index into @p args; advanced when a separate
        ///              value token is consumed.
        /// @param args  Full argument list.
        /// @return Ok on success, or an appropriate CliError.
        static CliResult<void> consume_short(
            const BaseCommand &cmd,
            ParseContext &ctx,
            std::string_view arg,
            size_t &i,
            std::span<const std::string_view> args);

    private:
        /// @brief Set a flag or increment a counter on @p ctx.
        ///
        /// If the option is a CountingOption, the stored int is incremented
        /// by 1; otherwise the option is treated as a boolean flag and set
        /// to true.
        ///
        /// @param opt  The matched option definition.
        /// @param ctx  Parse context to write into.
        static void apply_flag(const OptionDef *opt, ParseContext &ctx);
    };
}  // namespace pjh::cli

#endif
