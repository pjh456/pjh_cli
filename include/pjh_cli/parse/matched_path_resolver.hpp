#ifndef INCLUDE_PJH_CLI_PARSE_MATCHED_PATH_RESOLVER_HPP
#define INCLUDE_PJH_CLI_PARSE_MATCHED_PATH_RESOLVER_HPP

#include <string>
#include <vector>

namespace pjh::cli
{
    class BaseCommand;

    /// @brief Matched subcommand path as a list of command name strings.
    struct MatchedPath
    {
        std::vector<std::string> commands;
    };

    /// @brief Utility for resolving matched subcommand paths from a parsed
    ///        command tree.
    ///
    /// After Parser::parse_command() returns a ParseContext, the caller can
    /// obtain the deepest matched command via ctx.matched_command().  This
    /// class converts that raw pointer into human-consumable forms:
    ///   - A vector of command pointers along the matched path
    ///   - A space-separated path string (e.g. "config set")
    ///   - A structured MatchedPath struct
    ///
    /// All methods are static and take a const BaseCommand* (typically from
    /// ctx.matched_command()).  The class cannot be instantiated.
    ///
    /// Usage:
    /// @code
    ///   auto r = app.parse(argc, argv);
    ///   if (r.is_ok()) {
    ///       auto &ctx = r.unwrap();
    ///       // Full path as a string
    ///       std::cout << MatchedPathResolver::to_path_string(
    ///           ctx.matched_command()) << "\n";
    ///       // Individual path components
    ///       auto cmds = MatchedPathResolver::resolve_chain(
    ///           ctx.matched_command());
    ///   }
    /// @endcode
    class MatchedPathResolver
    {
    public:
        MatchedPathResolver() = delete;

        /// @brief Walk the parent chain from the matched command to the root
        ///        and return the subcommand path (root excluded).
        ///
        /// For a parse that matched `app db migrate`, this returns
        /// [db, migrate].  For a parse that matched the root command itself
        /// (no subcommand), this returns an empty vector.
        ///
        /// The returned pointers are to the actual command objects owned by
        /// the command tree; they remain valid as long as the tree lives.
        ///
        /// @param matched_cmd  The deepest matched command (from
        ///                      ctx.matched_command()).  May be nullptr.
        /// @return Vector of command pointers in top-down order, or empty
        ///         if @p matched_cmd is nullptr or is the root itself.
        static std::vector<const BaseCommand *> resolve_chain(
            const BaseCommand *matched_cmd);

        /// @brief Produce a human-readable subcommand path string.
        ///
        /// Joins the command names from resolve_chain() with single spaces.
        /// For `app config set` this returns "config set".
        /// For the root command (no subcommand) this returns an empty string.
        ///
        /// @param matched_cmd  The deepest matched command (from
        ///                      ctx.matched_command()).  May be nullptr.
        /// @return Space-separated path, or empty string if no subcommand
        ///         was matched.
        static std::string to_path_string(const BaseCommand *matched_cmd);

        /// @brief Produce a structured MatchedPath from the matched command.
        ///
        /// Calls resolve_chain() internally, then collects each command's
        /// name() into MatchedPath::commands.
        ///
        /// @param matched_cmd  The deepest matched command (from
        ///                      ctx.matched_command()).  May be nullptr.
        /// @return MatchedPath with command names in top-down order
        ///         (empty commands list when no subcommand matched).
        static MatchedPath to_path_info(const BaseCommand *matched_cmd);
    };

}  // namespace pjh::cli

#endif
