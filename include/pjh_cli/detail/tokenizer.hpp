#ifndef INCLUDE_PJH_CLI_DETAIL_TOKENIZER_HPP
#define INCLUDE_PJH_CLI_DETAIL_TOKENIZER_HPP

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "string_utils.hpp"

namespace pjh::cli::detail
{

    /// @brief Split @p input into tokens by whitespace.
    ///        Double-quoted spans are treated as a single token.
    ///        Quote characters are stripped from the output.
    inline std::vector<std::string> tokenize_input(std::string_view input)
    {
        std::vector<std::string> tokens;
        std::string tok;
        bool in_quote = false;

        for (size_t i = 0; i < input.size(); i++)
        {
            char c = input[i];
            if (c == '"')
            {
                in_quote = !in_quote;
                continue;
            }
            if (c == ' ' && !in_quote)
            {
                if (!tok.empty())
                {
                    tokens.push_back(std::move(tok));
                    tok.clear();
                }
                continue;
            }
            tok += c;
        }
        if (!tok.empty())
            tokens.push_back(std::move(tok));

        return tokens;
    }

    /// @brief Result of parse_long_option().
    struct ParsedLongOption
    {
        std::string_view name;          ///< Without -- prefix, without =value.
        std::string_view value;         ///< Empty if no = present.
        bool has_equals = false;        ///< true if an = separator was found.
        bool is_negation = false;       ///< name starts with "no-".
        std::string_view negated_name;  ///< name with "no-" stripped.
    };

    /// @brief Parse a long-option token like "--opt" or "--opt=val".
    ///        Strips the leading "--", splits on the first '=', and checks
    ///        for a "no-" negation prefix in the option name.
    inline ParsedLongOption parse_long_option(std::string_view token)
    {
        // Strip leading "--"
        auto arg = token;
        if (arg.size() > 2 && arg[0] == '-' && arg[1] == '-')
            arg = arg.substr(2);

        // Split on first '='
        auto sv = split_name_value(arg);

        // Check negation
        ParsedLongOption result;
        result.name = sv.name;
        result.value = sv.value;
        result.has_equals = sv.has_eq;

        if (result.name.size() > 3 && result.name[0] == 'n' && result.name[1] == 'o' &&
            result.name[2] == '-')
        {
            result.is_negation = true;
            result.negated_name = result.name.substr(3);
        }

        return result;
    }

}  // namespace pjh::cli::detail

#endif
