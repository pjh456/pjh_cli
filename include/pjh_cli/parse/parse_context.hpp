#ifndef INCLUDE_PJH_CLI_PARSE_CONTEXT_HPP
#define INCLUDE_PJH_CLI_PARSE_CONTEXT_HPP

#include <cstddef>
#include <filesystem>
#include <memory>
#include <pjh_cli/core/error.hpp>
#include <pjh_cli/core/fixed_string.hpp>
#include <pjh_cli/core/type.hpp>
#include <pjh_cli/detail/concept.hpp>
#include <pjh_result.hpp>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace pjh::cli
{
    class BaseCommand;

    /// @brief Container for parsed option and argument values.
    ///
    /// Values are indexed by compile-time key and retrieved via typed accessors.
    /// Storage uses per-type unordered_map inside a tuple — one map for each of
    /// the five builtin types (bool, int, double, string, path).
    ///
    /// Lookup follows the parent chain (for subcommand scoping): if a value is
    /// not found in the current context, the parent context is queried
    /// recursively.
    ///
    /// Key types:
    ///   - Named options:  `fixed_string("port")`
    ///   - Positional args: `0`, `1`, … (size_t index literal)
    ///
    /// Examples:
    /// @code
    ///   int port = ctx.get<int, fixed_string("port")>();
    ///   bool has_verbose = ctx.has<fixed_string("verbose")>();
    ///   auto src = ctx.get_or<std::string, 0>("default.txt");
    ///   auto maybe = ctx.try_get<int, fixed_string("timeout")>();
    /// @endcode
    class ParseContext
    {
        template <typename T>
        using IdMap = std::unordered_map<size_t, T>;
        template <typename T>
        using IdVecMap = IdMap<std::vector<T>>;

    public:
        /// @brief Retrieve a typed value by compile-time key.
        /// @tparam T Target type (must satisfy BuiltinType).
        /// @tparam Key fixed_string literal or size_t.
        /// @return Reference to the stored value.
        /// @throws LogicError if no value is stored for this key.
        template <detail::BuiltinType T, auto Key>
            requires detail::OptionKey<decltype(Key)>
        T &get()
        {
            constexpr auto h = key_hash(Key);
            if (auto *p = find_scalar<T>(h))
                return *p;
            throw LogicError("value not found for key");
        }

        /// @brief Const overload of get().
        template <detail::BuiltinType T, auto Key>
            requires detail::OptionKey<decltype(Key)>
        const T &get() const
        {
            constexpr auto h = key_hash(Key);
            if (auto *p = find_scalar<T>(h))
                return *p;
            throw LogicError("value not found for key");
        }

        /// @brief Check whether a value exists for the given key (walks parent chain).
        /// @tparam Key fixed_string or size_t.
        /// @return true if the key (or any parent) has a stored value.
        template <auto Key>
            requires detail::OptionKey<decltype(Key)>
        bool has() const noexcept
        {
            return has_in_chain(key_hash(Key));
        }

        /// @brief Try to retrieve a value; returns None if absent (no throw).
        /// @tparam T Target type.
        /// @tparam Key fixed_string or size_t.
        /// @return Option<T>::Some(value) if present, None() otherwise.
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
        /// @tparam T Target type.
        /// @tparam Key fixed_string or size_t.
        /// @param fallback  Default value when key is absent.
        /// @return Stored value, or @p fallback.
        template <detail::BuiltinType T, auto Key>
            requires detail::OptionKey<decltype(Key)>
        T get_or(T fallback) const
        {
            constexpr auto h = key_hash(Key);
            if (auto *p = find_scalar<T>(h))
                return *p;
            return fallback;
        }

        /// @brief Store a single value (used internally by parser).
        /// @tparam T Value type.
        /// @param hash  Runtime key hash.
        /// @param value The value to store.
        template <detail::BuiltinType T>
        void set_value(size_t hash, T value)
        {
            scalar_map<T>()[hash] = std::move(value);
            m_present.insert(hash);
        }

        /// @brief Check if a value exists by runtime hash (used internally).
        /// @param hash Runtime key hash.
        /// @return true if the key (or a parent) is present.
        bool has_value(size_t hash) const noexcept { return has_in_chain(hash); }

        /// @brief Get a typed value by runtime hash (used internally).
        /// @tparam T Value type.
        /// @param hash         Runtime key hash.
        /// @param default_val  Fallback when key is absent.
        /// @return Stored value or @p default_val.
        template <detail::BuiltinType T>
        T get_value(size_t hash, T default_val) const
        {
            if (auto *p = find_scalar<T>(hash))
                return *p;
            return default_val;
        }

        /// @brief Extra positional arguments collected under
        ///        ExtraArgsPolicy::Store.
        /// @return Vector of raw string tokens, in order.
        const std::vector<std::string> &extra_args() const noexcept
        {
            return m_extra_args;
        }

        /// @brief The leaf command matched during parsing.
        BaseCommand *matched_command() noexcept { return m_matched_cmd; }
        /// @brief Const overload.
        const BaseCommand *matched_command() const noexcept { return m_matched_cmd; }

        /// @brief True if --help / -h was encountered during parsing.
        ///
        /// When true the caller should print help_text() instead of
        /// executing the command action.
        bool help_requested() const noexcept { return !m_help_text.empty(); }

        /// @brief Pre-formatted help text from --help / -h handling.
        /// @return Multi-line help string, non-empty when help_requested().
        const std::string &help_text() const noexcept { return m_help_text; }

        /// @brief Set the pre-formatted help text (used internally by Parser).
        /// @param text Output from HelpFormatter::format_help().
        void set_help_text(std::string text) { m_help_text = std::move(text); }

        /// @brief Matched subcommand chain (root excluded).
        ///
        /// Each entry is a command along the matched path, in top-down order.
        /// For `app config set` this returns [config, set].
        /// @return Mutable vector of command pointers (empty if nothing matched).
        std::vector<BaseCommand *> matched_commands();
        /// @brief Const overload.
        std::vector<const BaseCommand *> matched_commands() const;

        /// @brief Full matched subcommand path as a space-separated string.
        /// @return e.g. "config set".
        std::string matched_path() const;

        /// @brief Set the deepest matched command (used internally by Parser).
        void set_matched_command(BaseCommand *cmd) { m_matched_cmd = cmd; }

        /// @brief Append a value for a repeatable option (used internally).
        /// @tparam T Value type.
        /// @param hash  Runtime key hash.
        /// @param value Value to append.
        template <detail::BuiltinType T>
        void append_value(size_t hash, T value)
        {
            vector_map<T>()[hash].push_back(std::move(value));
            m_present.insert(hash);
        }

        /// @brief Retrieve all values for a repeatable option.
        /// @tparam T Target type.
        /// @tparam Key fixed_string or size_t.
        /// @return Reference to the vector of stored values.
        /// @throws LogicError if no values are stored for this key.
        template <detail::BuiltinType T, auto Key>
            requires detail::OptionKey<decltype(Key)>
        const std::vector<T> &get_all() const
        {
            constexpr auto h = key_hash(Key);
            if (auto *p = find_vector<T>(h))
                return *p;
            throw LogicError("value not found for key");
        }

        /// @brief Append an unrecognised positional arg (used internally by Parser).
        /// @param s The raw token string.
        void add_extra_arg(std::string s) { m_extra_args.push_back(std::move(s)); }

        /// @brief Link a parent context for scoped lookup.
        ///
        /// When a subcommand creates a child ParseContext, the parent
        /// is set so that has() / get() on the child can fall through
        /// to parent option values.
        /// @param parent Shared pointer to the parent context.
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
        std::string m_help_text;
    };

}  // namespace pjh::cli

#endif  // INCLUDE_PJH_CLI_PARSE_CONTEXT_HPP
