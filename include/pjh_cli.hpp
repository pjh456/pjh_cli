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

#include <pjh_cli/core.hpp>
#include <pjh_cli/option.hpp>
#include <pjh_cli/command.hpp>
#include <pjh_cli/parse.hpp>
#include <pjh_cli/format.hpp>
#include <pjh_cli/console.hpp>
#include <pjh_cli/app.hpp>

#endif  // INCLUDE_PJH_CLI_HPP
