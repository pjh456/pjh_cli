#ifndef INCLUDE_PJH_CLI_OPTION_HPP
#define INCLUDE_PJH_CLI_OPTION_HPP

/// @brief Umbrella header for the option model.
///
/// Re-exports ArgDef / OptionDef, every typed option class (Bool, Count, Enum,
/// Float, Int, Path, Str), OptionBuilder, and the command_builder.hpp hub that
/// also carries OptionGroupBuilder / GroupMode.  Cross-cutting behavior mixins
/// live under <pjh_cli/option/mixin/>.

#include <pjh_cli/command/command_builder.hpp>
#include <pjh_cli/option/arg_def.hpp>
#include <pjh_cli/option/bool_option.hpp>
#include <pjh_cli/option/count_option.hpp>
#include <pjh_cli/option/enum_option.hpp>
#include <pjh_cli/option/float_option.hpp>
#include <pjh_cli/option/int_option.hpp>
#include <pjh_cli/option/option_builder.hpp>
#include <pjh_cli/option/option_def.hpp>
#include <pjh_cli/option/path_option.hpp>
#include <pjh_cli/option/str_option.hpp>

#endif  // INCLUDE_PJH_CLI_OPTION_HPP
