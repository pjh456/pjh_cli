#ifndef INCLUDE_PJH_CLI_DETAIL_HELP_FORMATTER_HPP
#define INCLUDE_PJH_CLI_DETAIL_HELP_FORMATTER_HPP

#include <functional>
#include <string>

namespace pjh::cli
{
    class BaseCommand;

    namespace detail
    {
        /// @brief Signature of an injectable batch help renderer.
        using HelpFormatterFn = std::function<std::string(const BaseCommand &)>;

        /// @brief Built-in batch help renderer, implemented by the format
        ///        layer (src/help.cpp).  Declared here so the parse layer can
        ///        default to it without including format/.
        std::string default_format_help(const BaseCommand &cmd);
    }  // namespace detail
}  // namespace pjh::cli

#endif  // INCLUDE_PJH_CLI_DETAIL_HELP_FORMATTER_HPP
