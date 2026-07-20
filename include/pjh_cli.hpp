#ifndef INCLUDE_PJH_CLI_HPP
#define INCLUDE_PJH_CLI_HPP

/// @brief pjh_cli — A modern C++20 CLI library with Rust-style error handling.
///
/// Features:
///   - Subcommand tree (composite pattern) with deep nesting
///   - Batch (argc/argv) and interactive (REPL) parsing modes
///   - Compile-time option keys via fixed_string NTTP
///   - Positional argument support by index
///   - Fuzzy command matching and tab completion
///   - Result<T, CliError> based error handling (no exceptions for parse errors)

#include "pjh_cli/app.hpp"
#include "pjh_cli/arg_def.hpp"
#include "pjh_cli/command.hpp"
#include "pjh_cli/console.hpp"
#include "pjh_cli/converter.hpp"
#include "pjh_cli/error.hpp"
#include "pjh_cli/fixed_string.hpp"
#include "pjh_cli/hint.hpp"
#include "pjh_cli/matcher.hpp"
#include "pjh_cli/option/bool_option.hpp"
#include "pjh_cli/option/count_option.hpp"
#include "pjh_cli/option/float_option.hpp"
#include "pjh_cli/option/int_option.hpp"
#include "pjh_cli/option/path_option.hpp"
#include "pjh_cli/option/str_option.hpp"
#include "pjh_cli/option_def.hpp"
#include "pjh_cli/parse_context.hpp"
#include "pjh_cli/parser.hpp"
#include "pjh_cli/token.hpp"
#include "pjh_cli/type.hpp"

#endif  // INCLUDE_PJH_CLI_HPP
