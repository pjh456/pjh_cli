#ifndef INCLUDE_PJH_CLI_HELP_FORMATTER_HPP
#define INCLUDE_PJH_CLI_HELP_FORMATTER_HPP

#include <pjh_cli/command/base_command.hpp>
#include <pjh_cli/format/info.hpp>
#include <string>
#include <string_view>

namespace pjh::cli
{

    /// @brief Utility for rendering help text from a command tree.
    ///
    /// Produces the standard CLI help output:
    ///   - One-line usage string
    ///   - Description
    ///   - Options table (with labels, required/default markers)
    ///   - Positional arguments table
    ///   - Subcommands table
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
        /// @param program_name  Display name for the program (empty → cmd.name()).
        /// @return Single-line string ending with no newline.
        static std::string format_usage(
            const BaseCommand &cmd, std::string_view program_name = "");

        /// @brief Format full help text for a command.
        ///
        /// Convenience wrapper that calls collect_help() then format_help(HelpInfo).
        ///
        /// @param cmd           The command to render.
        /// @param program_name  Display name.
        /// @return Multi-line help string.
        static std::string format_help(
            const BaseCommand &cmd, std::string_view program_name = "");

        /// @brief Walk a command tree and collect structured help data.
        ///
        /// Populates a HelpInfo with the program name, description, options,
        /// positional args (from leaf commands), and subcommands (respecting
        /// visibility + enabled predicates).
        ///
        /// @param cmd         The command to inspect.
        /// @param program_name  Display name.
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
        ///     -s, --long VALUE   description (required) (default: ...)
        ///
        ///   Arguments:
        ///     src              description (required)
        ///
        ///   Subcommands:
        ///     serve            Start the server
        ///
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
        /// The value placeholder is the option name uppercased.
        ///
        /// @param opt  Option metadata.
        /// @param sep  Separator between short and long names (default ", ").
        /// @return Formatted label string.
        static std::string option_label(
            const OptionInfo &opt, std::string_view sep = ", ");
    };

}  // namespace pjh::cli

#endif
