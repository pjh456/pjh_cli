#ifndef INCLUDE_PJH_CLI_INFO_HPP
#define INCLUDE_PJH_CLI_INFO_HPP

#include <pjh_cli/command/base_command.hpp>
#include <pjh_cli/core/type.hpp>
#include <pjh_cli/option/arg_def.hpp>
#include <pjh_cli/option/option_def.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace pjh::cli
{

    /// @brief Structured metadata about a registered option.
    ///
    /// Produced by collecting data from an OptionDef for use in
    /// help rendering and hint formatting.  All string_view fields
    /// alias the command tree's strings.
    struct OptionInfo
    {
        std::string_view long_name;           ///< Long option name without "--" prefix.
        char short_name = 0;                  ///< Short option character (0 if none).
        std::string_view description;         ///< Help text description.
        ValueTag value_tag = ValueTag::Bool;  ///< Type tag for display.
        bool has_value = false;               ///< Whether it consumes a CLI value token.
        bool is_required = false;             ///< Whether it must appear on the CLI.
        bool has_default = false;             ///< Whether a typed default is registered.
        std::string default_str;              ///< Human-readable default value string.
        bool is_negatable = false;            ///< Whether --no-xxx negation is enabled.
        bool is_counting = false;             ///< Whether it counts occurrences (-vvv).
        bool is_repeatable = false;  ///< Whether it accepts repeated / multi values.
        std::string_view env_var;    ///< Environment variable name (empty if none).

        OptionInfo() = default;

        /// @brief Construct from an OptionDef in the command tree.
        explicit OptionInfo(const OptionDef &opt) :
            long_name(opt.long_name()),
            short_name(opt.short_name()),
            description(opt.description()),
            value_tag(opt.value_tag()),
            has_value(opt.has_value()),
            is_required(opt.is_required()),
            has_default(opt.has_default()),
            default_str(opt.has_default() ? opt.default_value_str() : ""),
            is_negatable(opt.is_negatable()),
            is_counting(opt.is_counting()),
            is_repeatable(opt.is_repeatable()),
            env_var(opt.env_var())
        {
        }
    };

    /// @brief Structured metadata about a positional argument.
    struct ArgInfo
    {
        std::string_view name;         ///< Display name for help / error messages.
        std::string_view description;  ///< Help text description.
        bool is_required = false;      ///< Whether a value must be supplied.

        ArgInfo() = default;

        /// @brief Construct from an ArgDef.
        explicit ArgInfo(const ArgDef &arg) :
            name(arg.m_name), description(arg.m_description), is_required(arg.m_required)
        {
        }
    };

    /// @brief Structured metadata about a subcommand.
    struct SubcommandInfo
    {
        std::string_view name;         ///< Subcommand name (e.g. "serve").
        std::string_view description;  ///< Help text description.
    };

    /// @brief Structured help data for a command.
    ///
    /// Built by collect_help() and consumed by format_help(HelpInfo).
    /// All string_view fields point into the command tree and are valid
    /// as long as the tree exists (typically the program lifetime).
    struct HelpInfo
    {
        std::string_view program_name;            ///< Binary name for usage line.
        std::string_view description;             ///< Command description.
        std::vector<OptionInfo> options;          ///< Registered options.
        std::vector<ArgInfo> args;                ///< Positional arguments (leaf only).
        std::vector<SubcommandInfo> subcommands;  ///< Visible child subcommands.
    };

    /// @brief Structured context from walking a partial input through the command tree.
    ///
    /// Built by HintBuilder::build_context() and consumed by HintBuilder::format().
    struct HintContext
    {
        const BaseCommand *reached_command =
            nullptr;                          ///< Deepest command matched so far.
        size_t consumed_positional_args = 0;  ///< Number of positional tokens consumed.
        std::vector<OptionInfo> options;      ///< Options on the reached command.
        std::vector<ArgInfo> remaining_args;  ///< Unconsumed positional args.
    };

}  // namespace pjh::cli

#endif
