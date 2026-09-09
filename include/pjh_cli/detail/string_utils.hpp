#ifndef INCLUDE_PJH_CLI_DETAIL_STRING_UTILS_HPP
#define INCLUDE_PJH_CLI_DETAIL_STRING_UTILS_HPP

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <string>
#include <string_view>

namespace pjh::cli::detail
{

    /// @brief Hash functor for heterogenous lookup in unordered_map.
    ///
    /// Allows `std::unordered_map<std::string, T, transparent_string_hash>`
    /// to be queried with `string_view` keys without constructing
    /// a temporary `std::string`.
    struct transparent_string_hash
    {
        using is_transparent = void;

        /// @brief Hash a string_view by forwarding to std::hash<string_view>.
        /// @param sv  The string view to hash.
        /// @return Hash value.
        size_t operator()(std::string_view sv) const noexcept
        {
            return std::hash<std::string_view>{}(sv);
        }
    };

    /// @brief True when @p s is a dash-prefixed option token (--opt, -x,
    ///        --opt=val) rather than a value.
    ///
    /// Negative numbers such as -5, -3.14 and -.5 are NOT option tokens,
    /// nor is a bare "-".  Shared by Parser dispatch and OptionConsumer's
    /// value/greedy guards so the negative-number rule has one definition.
    ///
    /// @param s  Token to classify.
    /// @return true if @p s looks like an option, false otherwise.
    inline bool is_option_flag(std::string_view s) noexcept
    {
        return s.size() > 1 && s[0] == '-' &&
               !std::isdigit(static_cast<unsigned char>(s[1])) && s[1] != '.';
    }

    /// @brief True for a UTF-8 continuation byte (10xxxxxx).
    /// @param b  Byte to classify.
    /// @return true if @p b is a UTF-8 continuation byte.
    constexpr bool is_utf8_continuation_byte(unsigned char b) noexcept
    {
        return (b & 0xC0) == 0x80;
    }

    /// @brief Byte offset of the start of the UTF-8 code point ending at @p end.
    ///
    /// Walks back over continuation bytes and then over the lead byte, so the
    /// returned offset is always <= @p end.  Malformed input (a lone
    /// continuation byte or a run of them) degrades to the nearest byte
    /// boundary; the walk never reads outside `[0, s.size()]`.
    ///
    /// @param s    Byte string; need not be valid UTF-8.
    /// @param end  One-past-end offset, clamped to `[0, s.size()]`.
    /// @return Byte offset of the code-point start at or before @p end.
    inline std::size_t utf8_prev_code_point(std::string_view s, std::size_t end) noexcept
    {
        if (end > s.size())
            end = s.size();
        std::size_t i = end;
        while (i > 0 && is_utf8_continuation_byte(static_cast<unsigned char>(s[i - 1])))
            --i;
        return i > 0 ? i - 1 : 0;
    }

    /// @brief Number of UTF-8 code points in @p s.
    ///
    /// Counts every non-continuation byte (lead bytes and stray bytes); each
    /// continuation byte contributes nothing.  For ASCII this equals
    /// `s.size()`, so callers can use it unconditionally.
    ///
    /// @param s  Byte string; need not be valid UTF-8.
    /// @return Best-effort code-point count.
    inline std::size_t utf8_code_point_count(std::string_view s) noexcept
    {
        std::size_t n = 0;
        for (char c : s)
            if (!is_utf8_continuation_byte(static_cast<unsigned char>(c)))
                ++n;
        return n;
    }

    /// @brief General-purpose string manipulation utilities.
    ///
    /// Pure functions, no mutable state.  All methods operate on
    /// string_view inputs and return new strings or structs by value.
    class StringUtils
    {
    public:
        StringUtils() = delete;

        /// @brief Result of split_name_value().
        struct SplitNameValue
        {
            std::string_view
                name;  ///< Part before the first '=' (empty if string starts with '=').
            std::string_view value;  ///< Part after the first '=' (empty if no '='
                                     ///< present or '=' is at end).
            bool has_eq;             ///< true if an '=' character was found in the input.
        };

        /// @brief Convert a string to uppercase in-place (ASCII only).
        ///
        /// Non-ASCII characters are left unchanged.  The original string
        /// is not modified; a new uppercase string is returned.
        ///
        /// @param s  Input string view.
        /// @return A new string with all ASCII lowercase letters converted
        ///         to uppercase.
        static inline std::string to_upper_copy(std::string_view s)
        {
            std::string out(s);
            std::transform(
                out.begin(), out.end(), out.begin(),
                [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
            return out;
        }

        /// @brief Case-insensitive string equality (ASCII only).
        ///
        /// Compares two strings character-by-character after converting each
        /// to lowercase via std::tolower.  Strings of different lengths
        /// are never equal.
        ///
        /// @param a  First string.
        /// @param b  Second string.
        /// @return true if a and b are equal ignoring ASCII case.
        static constexpr bool case_insensitive_equal(
            std::string_view a, std::string_view b) noexcept
        {
            return a.size() == b.size() &&
                   std::equal(
                       a.begin(), a.end(), b.begin(),
                       [](char x, char y) noexcept
                       {
                           return std::tolower(static_cast<unsigned char>(x)) ==
                                  std::tolower(static_cast<unsigned char>(y));
                       });
        }

        /// @brief Split a string on the first '=' character.
        ///
        /// If no '=' is present, the entire string is returned as @p name
        /// and @p value is empty (has_eq = false).
        /// If an '=' is present, @p name is the portion before the first '=',
        /// @p value is the portion after (possibly empty).
        /// Only the first '=' is used as the delimiter.
        ///
        /// Examples:
        ///   `"port=8080"`    → { name="port",  value="8080", has_eq=true  }
        ///   `"--port=8080"`  → { name="--port", value="8080", has_eq=true  }
        ///   `"port"`         → { name="port",   value="",     has_eq=false }
        ///   `"a=b=c"`        → { name="a",      value="b=c",  has_eq=true  }
        ///   `"="`            → { name="",       value="",     has_eq=true  }
        ///
        /// @param arg  Input string to split.
        /// @return SplitNameValue with the split components.
        static inline SplitNameValue split_name_value(std::string_view arg) noexcept
        {
            auto eq = arg.find('=');
            if (eq == std::string_view::npos)
                return {arg, {}, false};
            return {arg.substr(0, eq), arg.substr(eq + 1), true};
        }
    };

}  // namespace pjh::cli::detail

#endif
