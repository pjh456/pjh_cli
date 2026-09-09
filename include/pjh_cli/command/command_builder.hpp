/// @file
/// Builder extension for BaseCommand: aggregates every typed option class
/// plus the OptionBuilder / OptionGroupBuilder method definitions.
///
/// `command/base_command.hpp` keeps only the builder declarations; include
/// this header (directly or through `command.hpp` / `app.hpp` / `option.hpp`)
/// at any call site that instantiates `.option()` or `.group()`.
///
/// `base_command.hpp` must be complete before the impl headers are parsed,
/// because their method bodies call `BaseCommand::add_option()` and
/// `BaseCommand::find_option_by_hash()`.

#ifndef INCLUDE_PJH_CLI_COMMAND_COMMAND_BUILDER_HPP
#define INCLUDE_PJH_CLI_COMMAND_COMMAND_BUILDER_HPP

#include <pjh_cli/command/base_command.hpp>
#include <pjh_cli/option/bool_option.hpp>
#include <pjh_cli/option/count_option.hpp>
#include <pjh_cli/option/enum_option.hpp>
#include <pjh_cli/option/float_option.hpp>
#include <pjh_cli/option/int_option.hpp>
#include <pjh_cli/option/option_builder.hpp>
#include <pjh_cli/option/option_builder_impl.hpp>
#include <pjh_cli/option/option_group_builder.hpp>
#include <pjh_cli/option/option_group_builder_impl.hpp>
#include <pjh_cli/option/path_option.hpp>
#include <pjh_cli/option/str_option.hpp>

#endif  // INCLUDE_PJH_CLI_COMMAND_COMMAND_BUILDER_HPP
