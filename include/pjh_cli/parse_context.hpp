#ifndef INCLUDE_PJH_CLI_PARSE_CONTEXT_HPP
#define INCLUDE_PJH_CLI_PARSE_CONTEXT_HPP

#include <cstddef>
#include <filesystem>
#include <memory>
#include <pjh_result.hpp>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "detail/concept.hpp"
#include "error.hpp"
#include "fixed_string.hpp"
#include "type.hpp"

namespace pjh::cli
{
    class BaseCommand;

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
        /// @throws LogicError if the key has no stored value.
        template <detail::BuiltinType T, auto Key>
            requires detail::OptionKey<decltype(Key)>
        T &get()
        {
            constexpr auto h = key_hash(Key);
            if (auto *p = find_scalar<T>(h))
                return *p;
            throw LogicError("value not found for key");
        }

        /// @brief Const overload.
        template <detail::BuiltinType T, auto Key>
            requires detail::OptionKey<decltype(Key)>
        const T &get() const
        {
            constexpr auto h = key_hash(Key);
            if (auto *p = find_scalar<T>(h))
                return *p;
            throw LogicError("value not found for key");
        }

        /// @brief Check if a value exists for the given compile-time key.
        template <auto Key>
            requires detail::OptionKey<decltype(Key)>
        bool has() const noexcept
        {
            return has_in_chain(key_hash(Key));
        }

        /// @brief Try to retrieve a value; returns None if absent (no throw).
        template <detail::BuiltinType T, auto Key>
            requires detail::OptionKey<decltype(Key)>
        pjh::result::Option<T> try_get() const
        {
            constexpr auto h = key_hash(Key);
            if (auto *p = find_scalar<T>(h))
                return pjh::result::Option<T>::Some(*p);
            return pjh::result::Option<T>::None();
        }

        /// @brief Retrieve a value or return @p fallback if absent (no throw).
        template <detail::BuiltinType T, auto Key>
            requires detail::OptionKey<decltype(Key)>
        T get_or(T fallback) const
        {
            constexpr auto h = key_hash(Key);
            if (auto *p = find_scalar<T>(h))
                return *p;
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
        bool has_value(size_t hash) const noexcept { return has_in_chain(hash); }

        /// @brief Get a value by runtime hash (used internally).
        template <detail::BuiltinType T>
        T get_value(size_t hash, T default_val) const
        {
            if (auto *p = find_scalar<T>(hash))
                return *p;
            return default_val;
        }

        /// @brief Extra positional arguments collected when ExtraArgsPolicy::Store.
        const std::vector<std::string> &extra_args() const noexcept
        {
            return m_extra_args;
        }

        /// @brief The leaf command matched during parsing.
        BaseCommand *matched_command() noexcept { return m_matched_cmd; }
        const BaseCommand *matched_command() const noexcept { return m_matched_cmd; }

        /// @brief Matched subcommand chain (root excluded), built from
        /// BaseCommand::parent().
        std::vector<BaseCommand *> matched_commands();
        std::vector<const BaseCommand *> matched_commands() const;

        /// @brief Full matched path, e.g. "server start".
        std::string matched_path() const;

        /// @brief Set the matched command (used internally by parser).
        void set_matched_command(BaseCommand *cmd) { m_matched_cmd = cmd; }

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
            if (auto *p = find_vector<T>(h))
                return *p;
            throw LogicError("value not found for key");
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

        // ── Chain lookup helpers ──

        template <detail::BuiltinType T>
        T *find_scalar(size_t hash) noexcept
        {
            auto it = scalar_map<T>().find(hash);
            if (it != scalar_map<T>().end())
                return &it->second;
            return m_parent ? m_parent->template find_scalar<T>(hash) : nullptr;
        }

        template <detail::BuiltinType T>
        const T *find_scalar(size_t hash) const noexcept
        {
            auto it = scalar_map<T>().find(hash);
            if (it != scalar_map<T>().end())
                return &it->second;
            return m_parent ? m_parent->template find_scalar<T>(hash) : nullptr;
        }

        template <detail::BuiltinType T>
        const std::vector<T> *find_vector(size_t hash) const noexcept
        {
            auto it = vector_map<T>().find(hash);
            if (it != vector_map<T>().end())
                return &it->second;
            return m_parent ? m_parent->template find_vector<T>(hash) : nullptr;
        }

        bool has_in_chain(size_t hash) const noexcept
        {
            if (m_present.contains(hash))
                return true;
            return m_parent ? m_parent->has_in_chain(hash) : false;
        }

        ScalarMaps m_scalars;
        VecMaps m_vectors;
        std::unordered_set<size_t> m_present;

        std::shared_ptr<ParseContext> m_parent;

        BaseCommand *m_matched_cmd = nullptr;
        std::vector<std::string> m_extra_args;
    };

}  // namespace pjh::cli

#endif  // INCLUDE_PJH_CLI_PARSE_CONTEXT_HPP
