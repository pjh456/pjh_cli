#ifndef INCLUDE_PJH_CLI_DETAIL_STRING_UTILS_HPP
#define INCLUDE_PJH_CLI_DETAIL_STRING_UTILS_HPP

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <string>
#include <string_view>

namespace pjh::cli::detail
{

    struct transparent_string_hash
    {
        using is_transparent = void;
        size_t operator()(std::string_view sv) const noexcept
        {
            return std::hash<std::string_view>{}(sv);
        }
    };

    class StringUtils
    {
    public:
        StringUtils() = delete;

        struct SplitNameValue
        {
            std::string_view name;
            std::string_view value;
            bool has_eq;
        };

        static inline std::string to_upper_copy(std::string_view s)
        {
            std::string out(s);
            std::transform(
                out.begin(), out.end(), out.begin(),
                [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
            return out;
        }

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
