#ifndef INCLUDE_PJH_CLI_PARSE_CONTEXT_HPP
#define INCLUDE_PJH_CLI_PARSE_CONTEXT_HPP

#include "error.hpp"
#include "fixed_string.hpp"

#include <pjh_result.hpp>

#include <any>
#include <string>
#include <string_view>
#include <unordered_map>

namespace pjh::cli
{
    class Command;

    /// @brief Container for parsed option and argument values.
    ///
    /// Values are stored by compile-time key hash and retrieved via get<T, Key>().
    /// - For named options:  get<int, fixed_string("port")>()
    /// - For positional args: get<std::string, 0>()
    class ParseContext
    {
    public:
        /// @brief Retrieve a value by its compile-time key.
        /// @tparam T Expected value type.
        /// @tparam Key Compile-time key (fixed_string for options, size_t for args).
        /// @throws ParseLogicError if the key has no stored value.
        /// @throws std::bad_any_cast if stored type does not match T.
        template <typename T, auto Key>
        T &
        get()
        {
            constexpr auto h = key_hash(Key);
            auto it = m_values.find(h);
            if (it == m_values.end())
                throw ParseLogicError(
                    "value not found for key");
            return std::any_cast<T &>(it->second);
        }

        /// @brief Const overload of get().
        template <typename T, auto Key>
        const T &
        get() const
        {
            constexpr auto h = key_hash(Key);
            auto it = m_values.find(h);
            if (it == m_values.end())
                throw ParseLogicError(
                    "value not found for key");
            return std::any_cast<const T &>(it->second);
        }

        /// @brief Non-throwing accessor. Returns None if key is absent.
        template <typename T, auto Key>
        pjh::result::Option<T>
        try_get() const
        {
            constexpr auto h = key_hash(Key);
            auto it = m_values.find(h);
            if (it == m_values.end())
                return pjh::result::Option<T>::None();
            return pjh::result::Option<T>::Some(
                std::any_cast<const T &>(it->second));
        }

        /// @brief Check if a value exists for the given compile-time key.
        template <auto Key>
        bool
        has() const noexcept
        {
            constexpr auto h = key_hash(Key);
            return m_values.contains(h);
        }

        /// @brief Store a value (used internally by parser).
        template <typename T>
        void
        set_value(size_t hash, T value)
        {
            m_values[hash] = std::move(value);
        }

        /// @brief Check if a value exists by runtime hash (used internally).
        bool
        has_value(size_t hash) const noexcept
        {
            return m_values.contains(hash);
        }

        /// @brief Extra positional arguments collected when ExtraArgsPolicy::Store.
        const std::vector<std::string> &
        extra_args() const noexcept { return m_extra_args; }

        /// @brief The leaf command matched during parsing.
        const Command *
        matched_command() const noexcept { return m_matched_cmd; }

        /// @brief Path of matched commands, e.g. "serve start".
        const std::string &
        matched_path() const noexcept { return m_matched_path; }

        /// @brief Set the matched command (used internally by parser).
        void
        set_matched_command(const Command *cmd)
        {
            m_matched_cmd = cmd;
        }

        /// @brief Set the matched command path (used internally by parser).
        void
        set_matched_path(std::string p)
        {
            m_matched_path = std::move(p);
        }

        /// @brief Append an extra positional arg (used internally by parser).
        void
        add_extra_arg(std::string s)
        {
            m_extra_args.push_back(std::move(s));
        }

    private:
        std::unordered_map<size_t, std::any> m_values;
        const Command *m_matched_cmd = nullptr;
        std::vector<std::string> m_extra_args;
        std::string m_matched_path;
    };

} // namespace pjh::cli

#endif // INCLUDE_PJH_CLI_PARSE_CONTEXT_HPP
