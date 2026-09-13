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

        /// @brief Separator predicate shared by the scanner and callers that
        ///        must recognise token boundaries (e.g. the completion cursor
        ///        scan).
        ///
        /// The separator set is exactly ASCII space and horizontal tab; runs
        /// collapse and leading/trailing separators produce no token.
        ///
        /// @param c  Byte to classify.
        /// @return true when @p c separates tokens outside quotes.
        static constexpr bool is_separator(char c) noexcept
        {
            return c == ' ' || c == '\t';
        }

        /// @brief Invoke @p fn for each token of @p input, in order.
        ///
        /// Single scanning implementation of the tokenize() grammar:
        /// - Separators are ASCII space and horizontal tab; runs collapse and
        ///   leading/trailing separators are ignored.
        /// - A double quote opens/closes a quoted span.  Separators inside a
        ///   span are literal and the quote characters are stripped; quoted and
        ///   unquoted spans concatenate into a single token.
        /// - A backslash escapes the next byte only when that byte is a double
        ///   quote, a backslash, a space, or a tab: the backslash is removed
        ///   and the byte is emitted literally.  Before any other byte (or at
        ///   end of input) the backslash is kept verbatim, so Windows paths
        ///   (`C:\Users\name`) and regex bodies (`\d+`) survive.  Single-quote
        ///   spans and C-style escapes (`\n`, `\t`, `\xNN`) are not supported.
        /// - An explicitly empty quoted token is preserved: `a "" b` yields
        ///   {a,"",b} and a lone `""` yields one empty token.
        /// - An unterminated quote is forgiving: the rest of the input becomes
        ///   one token (opening quote stripped); no error is reported.
        ///
        /// Any token containing a quote or an escaped byte takes the merge
        /// path: its cleaned text is built in a reusable scratch buffer and the
        /// callback receives it with @c merged == true, so the callee must copy
        /// the text before returning.  Unquoted, unescaped tokens are zero-copy
        /// views into @p input.
        ///
        /// @tparam Fn  Callable invocable with Token.
        /// @param input  Text to split.
        /// @param fn     Sink invoked once per token, including empty quoted
        ///               tokens.
        template <typename Fn>
        static void tokenize_each(std::string_view input, Fn &&fn)
        {
            std::string scratch;
            bool in_quote = false;
            bool merging = false;
            bool started = false;
            std::size_t start = 0;
            for (std::size_t i = 0; i < input.size(); ++i)
            {
                const char c = input[i];
                if (c == '\\' && i + 1 < input.size() && is_escapable(input[i + 1]))
                {
                    if (!merging)
                    {
                        scratch.assign(input.substr(start, i - start));
                        merging = true;
                    }
                    scratch += input[i + 1];
                    started = true;
                    ++i;
                    continue;
                }
                if (c == '"')
                {
                    if (!merging)
                    {
                        scratch.assign(input.substr(start, i - start));
                        merging = true;
                    }
                    in_quote = !in_quote;
                    started = true;
                    continue;
                }
                if (!in_quote && is_separator(c))
                {
                    if (started)
                    {
                        if (merging)
                        {
                            fn(Token{scratch, true});
                            scratch.clear();
                            merging = false;
                        }
                        else
                        {
                            fn(Token{input.substr(start, i - start), false});
                        }
                        started = false;
                    }
                    start = i + 1;
                    continue;
                }
                if (merging)
                    scratch += c;
                started = true;
            }
            if (started)
            {
                if (merging)
                    fn(Token{scratch, true});
                else
                    fn(Token{input.substr(start, input.size() - start), false});
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
        /// Same grammar as tokenize(): unquoted, unescaped tokens are views
        /// into @p input; quoted/escaped tokens (including empty quoted
        /// tokens) are owned copies in @ref TokenViews::owned.
        ///
        /// @param input  Text to split.
        /// @return Views into @p input plus any merged-token storage.
        static TokenViews tokenize_views(std::string_view input)
        {
            TokenViews out;
            // Each emitted token consumes at least one non-separator character,
            // so the non-separator count is an allocation-free token upper bound.
            std::size_t upper = 0;
            for (char c : input)
                if (!is_separator(c))
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

        /// @brief Split @p input into owned tokens.
        ///
        /// Separators are space and tab; double-quoted spans group separators
        /// and have their quotes stripped; a backslash escapes a double quote,
        /// backslash, space, or tab; an empty quoted token is preserved; and an
        /// unterminated quote consumes the rest of the input.  See
        /// @ref tokenize_each for the full grammar.
        ///
        /// @param input  Text to split.
        /// @return Tokens in input order.
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

    private:
        /// @brief True when a backslash before @p c removes the backslash and
        ///        emits @p c literally.
        ///
        /// The set is deliberately narrow — exactly the bytes the scanner
        /// would otherwise treat specially — so a backslash before any other
        /// byte (the Windows-path / regex case) stays verbatim.
        ///
        /// @param c  Byte following a backslash.
        /// @return true when @p c is escapable.
        static constexpr bool is_escapable(char c) noexcept
        {
            return c == '"' || c == '\\' || is_separator(c);
        }
    };

}  // namespace pjh::cli::detail

#endif
