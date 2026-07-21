#ifndef INCLUDE_PJH_CLI_INFO_HPP
#define INCLUDE_PJH_CLI_INFO_HPP

#include <string>
#include <string_view>
#include <vector>

#include "arg_def.hpp"
#include "command/base_command.hpp"
#include "option_def.hpp"
#include "type.hpp"

namespace pjh::cli
{

    /// @brief Structured information about a registered option.
    struct OptionInfo
    {
        std::string_view long_name;
        char short_name = 0;
        std::string_view description;
        ValueTag value_tag = ValueTag::Bool;
        bool has_value = false;
        bool is_required = false;
        bool has_default = false;
        std::string default_str;
        bool is_negatable = false;
        bool is_counting = false;
        std::string_view env_var;

        OptionInfo() = default;

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
            env_var(opt.env_var())
        {
        }
    };

    /// @brief Structured information about a positional argument.
    struct ArgInfo
    {
        std::string_view name;
        std::string_view description;
        bool is_required = false;

        ArgInfo() = default;

        explicit ArgInfo(const ArgDef &arg) :
            name(arg.m_name), description(arg.m_description), is_required(arg.m_required)
        {
        }
    };

    /// @brief Structured information about a subcommand.
    struct SubcommandInfo
    {
        std::string_view name;
        std::string_view description;
    };

    /// @brief Structured help data for a command.
    ///
    /// Built by collect_help() and consumed by format_help(HelpInfo).
    /// All string_view fields point into the command tree and are valid
    /// as long as the tree exists (typically the program lifetime).
    struct HelpInfo
    {
        std::string_view program_name;
        std::string_view description;
        std::vector<OptionInfo> options;
        std::vector<ArgInfo> args;
        std::vector<SubcommandInfo> subcommands;
    };

    /// @brief Structured context from walking a partial input through the command tree.
    ///
    /// Built by build_hint_context() and consumed by format_hint(HintContext).
    struct HintContext
    {
        const BaseCommand *reached_command = nullptr;
        size_t consumed_positional_args = 0;
        std::vector<OptionInfo> options;
        std::vector<ArgInfo> remaining_args;
    };

}  // namespace pjh::cli

#endif
