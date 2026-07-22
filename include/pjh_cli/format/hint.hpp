#ifndef INCLUDE_PJH_CLI_HINT_HPP
#define INCLUDE_PJH_CLI_HINT_HPP
#include <pjh_cli/command/base_command.hpp>
#include <pjh_cli/format/info.hpp>
#include <string>
#include <string_view>

namespace pjh::cli
{

    /// @brief Controls which options appear in the hint string.
    enum class HintOptionMode
    {
        All,       ///< Show all registered options (default).
        Required,  ///< Show only required options.
        None,      ///< Show no options (only positional args).
    };

    /// @brief Configuration for HintBuilder::format().
    struct HintConfig
    {
        HintOptionMode option_mode = HintOptionMode::All;
    };

    /// @brief Utility for building interactive hints.
    ///
    /// Walks the command tree from a partial input string, determines which
    /// options and positional arguments remain, and renders them as a
    /// space-separated hint string (e.g. "[INT:port] <src> <dst>").
    class HintBuilder
    {
    public:
        HintBuilder() = delete;

        /// @brief Human-readable type name for an option (e.g. "INT", "STR").
        static std::string option_type_name(const OptionDef &opt);

        /// @brief Walk a partial input string to determine the current command position.
        static HintContext build_context(const BaseCommand &root, std::string_view input);

        /// @brief Build structured hint data from context and config.
        ///
        /// Pre-computes display strings for options (respecting filter)
        /// and remaining positional arguments.
        static HintInfo build_hint(const HintContext &ctx, HintConfig config = {});

        /// @brief Render HintInfo as a space-separated hint string.
        static std::string format(const HintInfo &info);

        /// @brief Render HintContext as a hint string showing all available options and
        /// args.
        static std::string format(const HintContext &ctx);

        /// @brief Format an interactive hint for the command reached by parsing @p input.
        ///
        /// Internally calls build_context() + build_hint() + format(HintInfo).
        /// For direct access to the structured data, use build_context() directly.
        static std::string format(
            const BaseCommand &root, std::string_view input, HintConfig config = {});
    };

}  // namespace pjh::cli

#endif
