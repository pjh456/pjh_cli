#ifndef INCLUDE_PJH_CLI_PARSE_CONTEXT_HPP
#define INCLUDE_PJH_CLI_PARSE_CONTEXT_HPP

#include <filesystem>
#include <memory>
#include <pjh_result.hpp>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

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
    /// Storage uses per-type unordered_map stored in a tuple, one for each builtin type.
    /// - For named options:  get<int, fixed_string("port")>()
    /// - For positional args: get<std::string, 0>()
    /// @tparam T Must satisfy detail::BuiltinType<T>.
    class ParseContext
    {
        template <typename T>
        using IdMap = std::unordered_map<size_t, T>;
        template <typename T>
        using IdVecMap = IdMap<std::vector<T>>;

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
            auto &map = scalar_map<T>();
            auto it = map.find(h);
            if (it != map.end())
                return it->second;
            if (m_parent)
                return m_parent->template get<T, Key>();
            throw LogicError("value not found for key");
        }

        /// @brief Const overload of get().
        template <detail::BuiltinType T, auto Key>
            requires detail::OptionKey<decltype(Key)>
        const T &get() const
        {
            constexpr auto h = key_hash(Key);
            auto &map = scalar_map<T>();
            auto it = map.find(h);
            if (it != map.end())
                return it->second;
            if (m_parent)
                return m_parent->template get<T, Key>();
            throw LogicError("value not found for key");
        }

        /// @brief Check if a value exists for the given compile-time key.
        template <auto Key>
            requires detail::OptionKey<decltype(Key)>
        bool has() const noexcept
        {
            constexpr auto h = key_hash(Key);
            if (m_present.contains(h))
                return true;
            if (m_parent)
                return m_parent->template has<Key>();
            return false;
        }

        /// @brief Try to retrieve a value; returns None if absent (no throw).
        /// @tparam T Expected value type.
        /// @tparam Key Compile-time key.
        /// @return `Option<T>` — `Some(value)` if present, `None()` if absent.
        template <detail::BuiltinType T, auto Key>
            requires detail::OptionKey<decltype(Key)>
        pjh::result::Option<T> try_get() const
        {
            constexpr auto h = key_hash(Key);
            auto &map = scalar_map<T>();
            auto it = map.find(h);
            if (it != map.end())
                return pjh::result::Option<T>::Some(it->second);
            if (m_parent)
                return m_parent->template try_get<T, Key>();
            return pjh::result::Option<T>::None();
        }

        /// @brief Retrieve a value or return @p fallback if absent (no throw).
        /// @tparam T Expected value type.
        /// @tparam Key Compile-time key.
        /// @param fallback Default to use if the key has no stored value.
        /// @return The stored value, or @p fallback.
        template <detail::BuiltinType T, auto Key>
            requires detail::OptionKey<decltype(Key)>
        T get_or(T fallback) const
        {
            constexpr auto h = key_hash(Key);
            auto &map = scalar_map<T>();
            auto it = map.find(h);
            if (it != map.end())
                return it->second;
            if (m_parent)
                return m_parent->template get_or<T, Key>(fallback);
            return fallback;
        }

        /// @brief Store a value (used internally by parser).
        template <detail::BuiltinType T>
        void set_value(size_t hash, T value)
        {
            scalar_map<T>()[hash] = std::move(value);
            m_present.insert(hash);
        }

        /// @brief Check if a value exists by runtime hash (used internally).
        bool has_value(size_t hash) const noexcept
        {
            if (m_present.contains(hash))
                return true;
            if (m_parent)
                return m_parent->has_value(hash);
            return false;
        }

        /// @brief Get a value by runtime hash (used internally).
        /// @tparam T Expected value type.
        /// @param hash Runtime key hash.
        /// @param default_val Fallback if not found.
        template <detail::BuiltinType T>
        T get_value(size_t hash, T default_val) const
        {
            auto &map = scalar_map<T>();
            auto it = map.find(hash);
            if (it != map.end())
                return it->second;
            if (m_parent)
                return m_parent->template get_value<T>(hash, default_val);
            return default_val;
        }

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

        /// @brief Append a value for a repeatable option (used internally).
        template <detail::BuiltinType T>
        void append_value(size_t hash, T value)
        {
            vector_map<T>()[hash].push_back(std::move(value));
            m_present.insert(hash);
        }

        /// @brief Retrieve all values for a repeatable option.
        /// @throws LogicError if the key has no stored values.
        template <detail::BuiltinType T, auto Key>
            requires detail::OptionKey<decltype(Key)>
        const std::vector<T> &get_all() const
        {
            constexpr auto h = key_hash(Key);
            auto &vec = vector_map<T>();
            auto it = vec.find(h);
            if (it == vec.end())
                throw LogicError("value not found for key");
            return it->second;
        }

        /// @brief Append an extra positional arg (used internally by parser).
        void add_extra_arg(std::string s) { m_extra_args.push_back(std::move(s)); }

        /// @brief Set the parent context for chained lookup.
        void set_parent(std::shared_ptr<ParseContext> parent) noexcept
        {
            m_parent = std::move(parent);
        }

    private:
        using ScalarMaps = std::tuple<
            IdMap<bool>,
            IdMap<int>,
            IdMap<double>,
            IdMap<std::string>,
            IdMap<std::filesystem::path>>;

        using VecMaps = std::tuple<
            IdVecMap<bool>,
            IdVecMap<int>,
            IdVecMap<double>,
            IdVecMap<std::string>,
            IdVecMap<std::filesystem::path>>;

        template <detail::BuiltinType T>
        auto &scalar_map() noexcept
        {
            return std::get<detail::type_index_v<T>>(m_scalars);
        }
        template <detail::BuiltinType T>
        auto const &scalar_map() const noexcept
        {
            return std::get<detail::type_index_v<T>>(m_scalars);
        }
        template <detail::BuiltinType T>
        auto &vector_map() noexcept
        {
            return std::get<detail::type_index_v<T>>(m_vectors);
        }
        template <detail::BuiltinType T>
        auto const &vector_map() const noexcept
        {
            return std::get<detail::type_index_v<T>>(m_vectors);
        }

        ScalarMaps m_scalars;
        VecMaps m_vectors;
        std::unordered_set<size_t> m_present;

        std::shared_ptr<ParseContext> m_parent;

        const Command *m_matched_cmd = nullptr;
        std::vector<std::string> m_extra_args;
        std::string m_matched_path;
    };

}  // namespace pjh::cli

#endif  // INCLUDE_PJH_CLI_PARSE_CONTEXT_HPP
