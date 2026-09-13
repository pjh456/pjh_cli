#ifndef INCLUDE_PJH_CLI_PARSE_HPP
#define INCLUDE_PJH_CLI_PARSE_HPP

/// @brief Umbrella header for the parse pipeline.
///
/// Re-exports ParseContext (value storage + matched command), Parser (the
/// stateless single-pass parser), and MatchedPathResolver / MatchedPath
/// (convert a matched command into a path).  The internal pipeline helpers
/// (OptionConsumer, ValueWriter, SubcommandResolver, ParseFinalizer,
/// ParseContextWriter) are not re-exported; include their granular headers
/// directly.

#include <pjh_cli/parse/matched_path_resolver.hpp>
#include <pjh_cli/parse/parse_context.hpp>
#include <pjh_cli/parse/parser.hpp>

#endif  // INCLUDE_PJH_CLI_PARSE_HPP
