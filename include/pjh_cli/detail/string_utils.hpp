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
