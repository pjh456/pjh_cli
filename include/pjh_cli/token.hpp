#ifndef INCLUDE_PJH_CLI_TOKEN_HPP
#define INCLUDE_PJH_CLI_TOKEN_HPP

#include <string>
#include <vector>

namespace pjh::cli
{
    /// @brief Classification of a parsed token.
    enum class TokenKind
    {
        ShortOption,  ///< Single-dash short option, e.g. `-v`
        LongOption,   ///< Double-dash long option, e.g. `--verbose`
        Value,        ///< A bare positional value (after `--` or standalone)
        DoubleDash,   ///< The `--` terminator token
    };

    /// @brief A single token from the argument tokenizer.
    struct Token
    {
        TokenKind kind;     ///< Kind of this token
        std::string value;  ///< Token text (option name, value, or "--")
    };

}  // namespace pjh::cli

#endif  // INCLUDE_PJH_CLI_TOKEN_HPP
