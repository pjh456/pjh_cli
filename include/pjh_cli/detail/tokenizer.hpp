#ifndef INCLUDE_PJH_CLI_DETAIL_TOKENIZER_HPP
#define INCLUDE_PJH_CLI_DETAIL_TOKENIZER_HPP

#include <cstddef>
#include <deque>
#include <pjh_cli/detail/string_utils.hpp>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace pjh::cli::detail
{

    /// @brief Utility for tokenizing CLI input strings.
    class Tokenizer
    {
    public:
        Tokenizer() = delete;

        /// @brief Result of parse_long_option().
        struct LongOption
        {
            std::string_view name;          ///< Without -- prefix, without =value.
            std::string_view value;         ///< Empty if no = present.
            bool has_equals = false;        ///< true if an = separator was found.
            bool is_negation = false;       ///< name starts with "no-".
            std::string_view negated_name;  ///< name with "no-" stripped.
        };

        /// @brief One scanned token and whether its text needs owned storage.
        struct Token
        {
            std::string_view text;  ///< Quote-stripped token text.
            bool merged = false;    ///< true: @c text aliases the scanner
                                    ///< scratch, valid only for the duration
                                    ///< of the @ref tokenize_each call.
        };

        /// @brief Invoke @p fn for each token of @p input, in order.
        ///
        /// Single scanning implementation of the tokenize() grammar: splits
        /// on spaces outside double quotes, strips quote characters, and drops
        /// empty tokens.  Escapes and tabs are not handled (owned by the
        /// tokenizer-grammar task).  Any token containing a quote character
        /// takes the merge path: its quote-stripped text is copied into a
        /// reusable scratch buffer and the callback receives it with
        /// @c merged == true, so the callee must copy the text before
        /// returning.  Unquoted tokens are zero-copy views into @p input.
        ///
        /// @tparam Fn  Callable invocable with Token.
        /// @param input  Text to split.
        /// @param fn     Sink invoked once per non-empty token.
        template <typename Fn>
        static void tokenize_each(std::string_view input, Fn &&fn)
        {
            std::string scratch;
            bool in_quote = false;
            bool merging = false;
            std::size_t start = 0;
            for (std::size_t i = 0; i <= input.size(); ++i)
            {
                const bool end = i == input.size();
                const char c = end ? ' ' : input[i];
                if (!end && c == '"')
                {
                    in_quote = !in_quote;
                    if (!merging)
                    {
                        scratch.assign(input.substr(start, i - start));
                        merging = true;
                    }
                    continue;
                }
                if (end || (c == ' ' && !in_quote))
                {
                    if (merging)
                    {
                        if (!scratch.empty())
                            fn(Token{scratch, true});
                        merging = false;
                        scratch.clear();
                    }
                    else if (i > start)
                    {
                        fn(Token{input.substr(start, i - start), false});
                    }
                    start = i + 1;
                    continue;
                }
                if (merging)
                    scratch += c;
            }
        }

        /// @brief Views over the tokens of @p input plus owned merged tokens.
        struct TokenViews
        {
            std::vector<std::string_view> tokens;  ///< In input order.
            std::deque<std::string> owned;         ///< Backing store for merged tokens
                                                   ///< (references stable across
                                                   ///< push_back).
        };

        /// @brief tokenize() without an owned string per token.
        ///
        /// Same grammar as tokenize(): unquoted tokens are views into @p
        /// input, quoted tokens are owned copies in @ref TokenViews::owned.
        ///
        /// @param input  Text to split.
        /// @return Views into @p input plus any merged-token storage.
        static TokenViews tokenize_views(std::string_view input)
        {
            TokenViews out;
            // Each emitted token consumes at least one non-space character,
            // so the non-space count is an allocation-free token upper bound.
            std::size_t upper = 0;
            for (char c : input)
                if (c != ' ')
                    ++upper;
            out.tokens.reserve(upper);
            tokenize_each(
                input,
                [&](Token t)
                {
                    if (t.merged)
                    {
                        out.owned.emplace_back(t.text);
                        out.tokens.push_back(out.owned.back());
                    }
                    else
                    {
                        out.tokens.push_back(t.text);
                    }
                });
            return out;
        }

        /// @brief Split @p input into owned tokens by whitespace.
        ///        Double-quoted spans are treated as a single token.
        ///        Quote characters are stripped from the output.
        static std::vector<std::string> tokenize(std::string_view input)
        {
            std::vector<std::string> tokens;
            tokenize_each(input, [&](Token t) { tokens.emplace_back(t.text); });
            return tokens;
        }

        /// @brief Parse a long-option token like "--opt" or "--opt=val".
        ///        Strips the leading "--", splits on the first '=', and checks
        ///        for a "no-" negation prefix in the option name.
        static LongOption parse_long_option(std::string_view token)
        {
            // Strip leading "--"
            auto arg = token;
            if (arg.size() > 2 && arg[0] == '-' && arg[1] == '-')
                arg = arg.substr(2);

            // Split on first '='
            auto sv = StringUtils::split_name_value(arg);

            // Check negation
            LongOption result;
            result.name = sv.name;
            result.value = sv.value;
            result.has_equals = sv.has_eq;

            if (result.name.size() > 3 && result.name[0] == 'n' &&
                result.name[1] == 'o' && result.name[2] == '-')
            {
                result.is_negation = true;
                result.negated_name = result.name.substr(3);
            }

            return result;
        }
    };

}  // namespace pjh::cli::detail

#endif
