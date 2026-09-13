#ifndef INCLUDE_PJH_CLI_CORE_HPP
#define INCLUDE_PJH_CLI_CORE_HPP

/// @brief Umbrella header for the foundational types.
///
/// Re-exports compile-time keys (fixed_string), result / failure aliases and
/// the builtin type table (type.hpp), structured errors (error.hpp), and
/// string converters (converter.hpp).  This layer has no upward dependencies.

#include <pjh_cli/core/converter.hpp>
#include <pjh_cli/core/error.hpp>
#include <pjh_cli/core/fixed_string.hpp>
#include <pjh_cli/core/type.hpp>

#endif  // INCLUDE_PJH_CLI_CORE_HPP
