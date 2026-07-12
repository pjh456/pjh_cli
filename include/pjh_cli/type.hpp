#ifndef INCLUDE_PJH_CLI_TYPE_HPP
#define INCLUDE_PJH_CLI_TYPE_HPP

#include <pjh_result.hpp>

#include "error.hpp"

namespace pjh::cli
{
    /// @brief Result type with ParseError as the error variant.
    ///
    /// Wraps pjh::result::Result<T, ParseError> for CLI parse operations.
    /// @tparam T Success type.
    template <typename T>
    using ParseResult =
        pjh::result::Result<T, ParseError>;

    /// @brief Convenience alias for returning ParseError from Result-returning functions.
    ///
    /// Usage: `return ParseFailure{ParseError("...")};`
    /// Implicitly converts to any ParseResult<T> with matching error type.
    using ParseFailure =
        pjh::result::Failure<ParseError>;

} // namespace pjh::cli

#endif // INCLUDE_PJH_CLI_TYPE_HPP
