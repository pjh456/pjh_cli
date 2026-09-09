#ifndef INCLUDE_PJH_CLI_PARSE_FINALIZER_HPP
#define INCLUDE_PJH_CLI_PARSE_FINALIZER_HPP

#include <pjh_cli/command/base_command.hpp>
#include <pjh_cli/core/type.hpp>
#include <pjh_cli/parse/parse_context.hpp>
#include <vector>

namespace pjh::cli
{
    /// @brief Post-parse validation and value finalisation.
    ///
    /// Called once after the main parse loop completes.  Walks the full
    /// command chain from root to the deepest matched command and performs
    /// the following steps in order:
    ///   1. If an option has an env-var and no CLI value, read from env.
    ///   2. Apply compile-time default values for options not yet set.
    ///   3. If an option is required and still not set, return an error.
    ///   4. Validate option groups (exactly-one / at-most-one / at-least-one).
    ///   5. If a positional arg is required and not set, return an error.
    ///
    /// Values resolve with CLI > env > default precedence: env fills unset
    /// options first, then defaults fill whatever env did not.
    ///
    /// The public entry point is finalize(); the five steps are decomposed
    /// into private static methods that can be tested individually.
    class ParseFinalizer
    {
    public:
        ParseFinalizer() = delete;

        /// @brief Finalise a parse result by applying env-vars, defaults,
        ///        required checks, and group validation.
        ///
        /// @param cmd  The deepest matched command.
        /// @param ctx  Parse context (parent chain already linked).
        /// @return Ok with the finalised context, or Err on validation failure.
        /// @throws LogicError if @p cmd is null.
        static CliResult<ParseContext> finalize(BaseCommand *cmd, ParseContext ctx);

        /// @brief Apply registered defaults for @p cmd's options that are
        ///        still unset.
        ///
        /// For each option with has_default() true and no value present,
        /// calls OptionDef::default_option_value() and stores the result.
        /// Replaces the former BaseCommand::apply_defaults() so the command
        /// layer no longer touches ParseContext storage.
        /// @param cmd Command whose options to apply defaults for.
        /// @param ctx Parse context to write into.
        /// @return Ok, or the default-validation error verbatim.
        static CliResult<void> apply_defaults(const BaseCommand &cmd, ParseContext &ctx);

    private:
        /// @brief Apply default values for every option along @p chain.
        ///
        /// Iterates each command in the chain and delegates to
        /// apply_defaults().
        static CliResult<void> apply_chain_defaults(
            const std::vector<BaseCommand *> &chain, ParseContext &ctx);

        /// @brief Fall back to environment variables for unset options.
        ///
        /// Reads the env snapshot from the root command.  For each option
        /// that has a non-empty env_var() and no value has been set, looks
        /// up the environment variable and calls
        /// ValueWriter::apply_option_raw().
        static CliResult<void> apply_chain_env(
            const std::vector<BaseCommand *> &chain, ParseContext &ctx);

        /// @brief Check that every required option has been set.
        ///
        /// Iterates all options on all commands in the chain.  If any option
        /// with is_required() true is still absent from @p ctx, returns a
        /// missing_required_option error.
        static CliResult<void> check_required_options(
            const std::vector<BaseCommand *> &chain, ParseContext &ctx);

        /// @brief Validate option group constraints.
        ///
        /// For each registered group on each command in the chain, checks
        /// that the constraint (ExactlyOne / AtMostOne / AtLeastOne) is
        /// satisfied.  Returns conflicting_options or required_option_group
        /// errors on violation.
        static CliResult<void> validate_groups(
            const std::vector<BaseCommand *> &chain, ParseContext &ctx);

        /// @brief Check required positional arguments on the leaf command.
        ///
        /// If @p cmd is a leaf, iterates its positional args and returns
        /// a missing_required_arg error for any with m_required true that
        /// have no stored value.
        static CliResult<void> check_required_args(BaseCommand *cmd, ParseContext &ctx);
    };
}  // namespace pjh::cli

#endif
