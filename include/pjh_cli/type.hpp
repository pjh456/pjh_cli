#ifndef INCLUDE_PJH_CLI_TYPE_HPP
#define INCLUDE_PJH_CLI_TYPE_HPP

#include <pjh_result.hpp>

#include "error.hpp"

namespace pjh::cli
{
    /// @brief Result type with CliError as the error variant.
    ///
    /// Wraps pjh::result::Result<T, CliError> for CLI parse operations.
    /// @tparam T Success type.
    template <typename T>
    using CliResult =
        pjh::result::Result<T, CliError>;

    /// @brief Convenience alias for returning CliError from Result-returning functions.
    ///
    /// Usage: `return CliFailure{CliError("...")};`
    /// Implicitly converts to any CliResult<T> with matching error type.
    using CliFailure =
        pjh::result::Failure<CliError>;

} // namespace pjh::cli

#endif // INCLUDE_PJH_CLI_TYPE_HPP
