#ifndef INCLUDE_PJH_CLI_FORMAT_HPP
#define INCLUDE_PJH_CLI_FORMAT_HPP

/// @brief Umbrella header for the formatting layer.
///
/// Re-exports help / usage rendering (HelpFormatter), interactive hints
/// (HintBuilder), fuzzy matching, subcommand listing and completion (matcher
/// free functions), console output strings (ConsoleOutput), and all info.hpp
/// DTOs (HelpInfo, HintInfo, CompletionResult, SuggestionInfo, ...).

#include <pjh_cli/format/console_output.hpp>
#include <pjh_cli/format/help_formatter.hpp>
#include <pjh_cli/format/hint.hpp>
#include <pjh_cli/format/info.hpp>
#include <pjh_cli/format/matcher.hpp>

#endif  // INCLUDE_PJH_CLI_FORMAT_HPP
