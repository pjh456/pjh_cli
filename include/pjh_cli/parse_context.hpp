#ifndef INCLUDE_PJH_CLI_PARSE_CONTEXT_HPP
#define INCLUDE_PJH_CLI_PARSE_CONTEXT_HPP

#include <filesystem>
#include <pjh_result.hpp>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include "detail/concept.hpp"
#include "error.hpp"
#include "fixed_string.hpp"
#include "type.hpp"

namespace pjh::cli
{
    class Command;

    /// @brief Container for parsed option and argument values.
    ///
    /// Values are stored by compile-time key hash and retrieved via get<T, Key>().
    /// Storage uses per-type unordered_map, one for each builtin type.
    /// - For named options:  get<int, fixed_string("port")>()
    /// - For positional args: get<std::string, 0>()
    /// @tparam T Must satisfy detail::BuiltinType<T>.
    class ParseContext
    {
    public:
        /// @brief Retrieve a value by its compile-time key.
        /// @tparam T Expected value type (must match the registered option/arg type).
        /// @tparam Key Compile-time key (fixed_string for options, size_t for args).
        /// @throws LogicError if the key has no stored value.
        template <detail::BuiltinType T, auto Key>
            requires detail::OptionKey<decltype(Key)>
        T &get()
        {
            constexpr auto h = key_hash(Key);
            auto &map = typed_map<T>();
            auto it = map.find(h);
            if (it == map.end())
                throw LogicError("value not found for key");
            return it->second;
        }

        /// @brief Const overload of get().
        template <detail::BuiltinType T, auto Key>
            requires detail::OptionKey<decltype(Key)>
        const T &get() const
        {
            constexpr auto h = key_hash(Key);
            auto &map = typed_map<T>();
            auto it = map.find(h);
            if (it == map.end())
                throw LogicError("value not found for key");
            return it->second;
        }

        /// @brief Check if a value exists for the given compile-time key.
        template <auto Key>
            requires detail::OptionKey<decltype(Key)>
        bool has() const noexcept
        {
            constexpr auto h = key_hash(Key);
            return m_present.contains(h);
        }

        /// @brief Store a value (used internally by parser).
        template <detail::BuiltinType T>
        void set_value(size_t hash, T value)
        {
            typed_map<T>()[hash] = std::move(value);
            m_present.insert(hash);
        }

        /// @brief Check if a value exists by runtime hash (used internally).
        bool has_value(size_t hash) const noexcept { return m_present.contains(hash); }

        /// @brief Extra positional arguments collected when ExtraArgsPolicy::Store.
        const std::vector<std::string> &extra_args() const noexcept
        {
            return m_extra_args;
        }

        /// @brief The leaf command matched during parsing.
        const Command *matched_command() const noexcept { return m_matched_cmd; }

        /// @brief Path of matched commands, e.g. "serve start".
        const std::string &matched_path() const noexcept { return m_matched_path; }

        /// @brief Set the matched command (used internally by parser).
        void set_matched_command(const Command *cmd) { m_matched_cmd = cmd; }

        /// @brief Set the matched command path (used internally by parser).
        void set_matched_path(std::string p) { m_matched_path = std::move(p); }

        /// @brief Append an extra positional arg (used internally by parser).
        void add_extra_arg(std::string s) { m_extra_args.push_back(std::move(s)); }

    private:
        template <detail::BuiltinType T>
        auto &typed_map() noexcept;
        template <detail::BuiltinType T>
        auto const &typed_map() const noexcept;

        std::unordered_map<size_t, bool> m_bool;
        std::unordered_map<size_t, int> m_int;
        std::unordered_map<size_t, double> m_double;
        std::unordered_map<size_t, std::string> m_string;
        std::unordered_map<size_t, std::filesystem::path> m_path;
        std::unordered_set<size_t> m_present;

        const Command *m_matched_cmd = nullptr;
        std::vector<std::string> m_extra_args;
        std::string m_matched_path;
    };

    // ── typed_map implementations ──

    template <detail::BuiltinType T>
    auto &ParseContext::typed_map() noexcept
    {
        if constexpr (std::same_as<T, bool>)
            return m_bool;
        else if constexpr (std::same_as<T, int>)
            return m_int;
        else if constexpr (std::same_as<T, double>)
            return m_double;
        else if constexpr (std::same_as<T, std::string>)
            return m_string;
        else if constexpr (std::same_as<T, std::filesystem::path>)
            return m_path;
    }

    template <detail::BuiltinType T>
    auto const &ParseContext::typed_map() const noexcept
    {
        if constexpr (std::same_as<T, bool>)
            return m_bool;
        else if constexpr (std::same_as<T, int>)
            return m_int;
        else if constexpr (std::same_as<T, double>)
            return m_double;
        else if constexpr (std::same_as<T, std::string>)
            return m_string;
        else if constexpr (std::same_as<T, std::filesystem::path>)
            return m_path;
    }

}  // namespace pjh::cli

#endif  // INCLUDE_PJH_CLI_PARSE_CONTEXT_HPP
