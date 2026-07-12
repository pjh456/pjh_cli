#ifndef INCLUDE_PJH_CLI_PARSE_CONTEXT_HPP
#define INCLUDE_PJH_CLI_PARSE_CONTEXT_HPP

#include "fixed_string.hpp"

#include <any>
#include <string>
#include <string_view>
#include <unordered_map>

namespace pjh::cli
{
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
        /// @throws std::bad_any_cast if type mismatch, std::out_of_range if missing.
        template <typename T, auto Key>
        T &
        get()
        {
            constexpr auto h = key_hash(Key);
            auto it = m_values.find(h);
            if (it == m_values.end())
                throw std::out_of_range(
                    "value not found for key");
            return std::any_cast<T &>(it->second);
        }

        /// @brief Check if a value exists for the given compile-time key.
        template <auto Key>
        bool
        has() const
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
        has_value(size_t hash) const
        {
            return m_values.contains(hash);
        }

        /// @brief Path of matched commands, e.g. "serve start".
        const std::string &
        matched_path() const { return m_matched_path; }

        /// @brief Set the matched command path (used internally by parser).
        void
        set_matched_path(std::string p)
        {
            m_matched_path = std::move(p);
        }

    private:
        std::unordered_map<size_t, std::any> m_values;
        std::string m_matched_path;
    };

} // namespace pjh::cli

#endif // INCLUDE_PJH_CLI_PARSE_CONTEXT_HPP
