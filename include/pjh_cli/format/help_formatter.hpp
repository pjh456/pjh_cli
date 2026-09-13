#ifndef INCLUDE_PJH_CLI_HELP_FORMATTER_HPP
#define INCLUDE_PJH_CLI_HELP_FORMATTER_HPP

#include <cstddef>
#include <pjh_cli/command/base_command.hpp>
#include <pjh_cli/format/info.hpp>
#include <sstream>
#include <string>
#include <string_view>

namespace pjh::cli
{

    /// @brief Utility for rendering help text from a command tree.
    ///
    /// Produces the standard CLI help output:
    ///   - One-line usage string (current command's options only)
    ///   - Description
    ///   - Options table (with labels and metadata annotations)
    ///   - Inherited Options table (options accepted from ancestors)
    ///   - Positional arguments table
    ///   - Subcommands table
    ///
    /// The Options table annotates `(required)`, `(env: VAR)`, `(default: X)`,
    /// `(negatable)`, `(counting)`, and `(repeatable)` after the description.
    /// Inherited rows reuse the same annotations and ordering.
    ///
    /// Two pipelines exist:
    ///   1. collect_help() → format_help(HelpInfo) for the full multi-line help
    ///   2. format_usage() for the compact one-liner
    ///
    /// All string_view fields in HelpInfo / OptionInfo point into the
    /// command tree and are valid for the tree's lifetime.
    ///
    /// Usage:
    /// @code
    ///   std::cout << HelpFormatter::format_help(app, "myapp");
    /// @endcode
    class HelpFormatter
    {
    public:
        HelpFormatter() = delete;

        /// @brief Format a one-line usage string.
        ///
        /// Example output: `"Usage: myapp [--port PORT] <src> <dst> <command>"`
        ///
        /// Walks the command tree directly (not through HelpInfo).  Options
        /// are shown with `[]` for optional, bare for required.  Positional
        /// args use `<>`.  If the command is a branch with visible
        /// subcommands, `<command>` is appended.
        ///
        /// @param cmd           The command to render.
        /// @param program_name  Display name for the program.  When empty, the
        ///                      full command path (root name + ancestors +
        ///                      cmd.name()) is derived from the tree.
        /// @return Single-line string ending with no newline.
        static std::string format_usage(
            const BaseCommand &cmd, std::string_view program_name = "");

        /// @brief Format full help text for a command.
        ///
        /// Convenience wrapper that calls collect_help() then format_help(HelpInfo).
        ///
        /// @param cmd           The command to render.
        /// @param program_name  Display name.  When empty, the full command
        ///                      path (root name + ancestors + cmd.name()) is
        ///                      derived from the tree.
        /// @return Multi-line help string.
        static std::string format_help(
            const BaseCommand &cmd, std::string_view program_name = "");

        /// @brief Walk a command tree and collect structured help data.
        ///
        /// Populates a HelpInfo with the program name, description, options
        /// registered on @p cmd, inherited ancestor options (nearest first),
        /// positional args (from leaf commands), and subcommands (respecting
        /// visibility + enabled predicates).
        ///
        /// @param cmd         The command to inspect.
        /// @param program_name  Display name.  Stored as a view into the
        ///                      caller's buffer, which must outlive the
        ///                      returned HelpInfo.  An empty value is stored
        ///                      as-is; this function does not derive a path.
        /// @param visibility  Visibility filter (default Both).
        /// @return A HelpInfo struct whose string_view members alias the command
        ///         tree's strings — valid as long as the tree lives.
        static HelpInfo collect_help(
            const BaseCommand &cmd,
            std::string_view program_name = "",
            Visibility visibility = Visibility::Both);

        /// @brief Render pre-collected help data as a human-readable string.
        ///
        /// Produces:
        ///   Usage: program_name [options] <args> <command>
        ///   description
        ///
        ///   Options:
        ///     -p, --port PORT     Port number (env: APP_PORT) (default: 8080)
        ///     -c, --compress      Compress (negatable)
        ///     -v, --verbose       Verbose (counting)
        ///     -I, --include PATH  Include path (repeatable)
        ///
        ///   Inherited Options:
        ///     -g, --global        Option declared on an ancestor command
        ///
        ///   Arguments:
        ///     src              description (required)
        ///
        ///   Subcommands:
        ///     serve            Start the server
        ///
        /// The usage line lists only the current command's options; inherited
        /// options appear in their own section.
        ///
        /// Option annotations are emitted in a fixed order after the
        /// description: `(required)`, `(env: VAR)`, `(default: X)`,
        /// `(negatable)`, `(counting)`, `(repeatable)`.
        /// Column widths auto-size up to 32 for options and 28 for args/subcommands.
        ///
        /// @param info  Structured data from collect_help().
        /// @return Multi-line help string.
        static std::string format_help(const HelpInfo &info);

        /// @brief Build a HelpDocument from structured HelpInfo.
        ///
        /// Pre-computes display labels and groups data into sections.
        /// The returned document is ready for format_help(HelpDocument).
        static HelpDocument build_document(const HelpInfo &info);

        /// @brief Render a HelpDocument as a multi-line help string.
        static std::string format_help(const HelpDocument &doc);

        /// @brief Build a UsageInfo from HelpInfo.
        ///
        /// Pre-computes token display strings for the usage line.
        static UsageInfo build_usage(const HelpInfo &info);

        /// @brief Render a UsageInfo as a one-line usage string.
        static std::string format_usage(const UsageInfo &info);

    private:
        /// @brief Full command path from the root to @p cmd, space-joined.
        ///
        /// Walks parent() links up to the root and joins non-empty canonical
        /// names from root to leaf.  Used as the default program name when the
        /// BaseCommand adapters receive an empty @c program_name.
        ///
        /// @param cmd  The command whose path to derive.
        /// @return Space-joined path, e.g. "myapp server start"; empty when
        ///         no ancestor (including @p cmd) has a name.
        static std::string command_path(const BaseCommand &cmd);

        /// @brief Write one line of a help section with padded left column.
        ///
        /// Output: `"  <left>  <right>\n"` where `<left>` is padded to
        /// @p left_width characters with spaces.
        ///
        /// @param os         Output stream.
        /// @param left       Left column text (e.g. "-p, --port PORT").
        /// @param right      Right column text (e.g. "The port number").
        /// @param left_width  Target width for the left column (padded with spaces).
        static void append_help_line(
            std::ostringstream &os,
            const std::string &left,
            const std::string &right,
            size_t left_width);

        /// @brief Build a display label for an option.
        ///
        /// Examples:
        ///   - short only:     `"-v"`
        ///   - long only:      `"--verbose"`
        ///   - both:           `"-v, --verbose"`
        ///   - with value:     `"-p, --port PORT"` or `"-p PORT"` / `"--port PORT"`
        ///
        /// The value placeholder is the option name uppercased.  The label is
        /// syntax only (no metadata); `(required)` / `(env: ...)` /
        /// `(default: ...)` / `(negatable)` / `(counting)` / `(repeatable)` are
        /// rendered in the right column by format_help(HelpInfo).
        ///
        /// @param opt  Option metadata.
        /// @param sep  Separator between short and long names (default ", ").
        /// @return Formatted label string.
        static std::string option_label(
            const OptionInfo &opt, std::string_view sep = ", ");
    };

}  // namespace pjh::cli

#endif
