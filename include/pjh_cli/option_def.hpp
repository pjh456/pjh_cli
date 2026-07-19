#ifndef INCLUDE_PJH_CLI_OPTION_DEF_HPP
#define INCLUDE_PJH_CLI_OPTION_DEF_HPP

#include <functional>
#include <pjh_result.hpp>
#include <string>
#include <string_view>
#include <vector>

#include "type.hpp"

namespace pjh::cli
{
    class ParseContext;

    /// @brief Definition of a named option (flag or valued).
    ///
    /// Identified by compile-time fixed_string key. Bool options are flags
    /// (no value consumed); all other types consume the next token as value.
    class OptionDef
    {
    public:
        /// @brief Default constructor (initializes m_default_str to None).
        OptionDef() : m_default_str(pjh::result::Option<std::string>::None()) {}

        // ── Getters ──

        /// @brief Get the long option name (e.g. "verbose").
        const std::string &long_name() const noexcept { return m_long_name; }

        /// @brief Get the short option character (0 if none).
        char short_name() const noexcept { return m_short_name; }

        /// @brief Get the help text description.
        const std::string &description() const noexcept { return m_description; }

        /// @brief Whether this option consumes a value token.
        bool has_value() const noexcept { return m_has_value; }

        /// @brief Whether this option must be provided on the command line.
        bool is_required() const noexcept { return m_required; }

        /// @brief Get the compile-time hash used to index ParseContext.
        size_t key_hash() const noexcept { return m_key_hash; }

        /// @brief Get the runtime type tag.
        ValueTag value_tag() const noexcept { return m_value_tag; }

        /// @brief Whether a default value has been registered.
        bool has_default() const noexcept { return m_default_str.is_some(); }

        /// @brief Get the default value string (empty if no default).
        const std::string &default_str() const noexcept { return m_default_str.unwrap(); }

        // ── Setters ──

        /// @brief Set the long option name.
        OptionDef &set_long_name(const std::string &s)
        {
            m_long_name = s;
            return *this;
        }

        /// @brief Set the long option name (move).
        OptionDef &set_long_name(std::string &&s)
        {
            m_long_name = std::move(s);
            return *this;
        }

        /// @brief Set the short option character (0 for none).
        OptionDef &set_short_name(char c)
        {
            m_short_name = c;
            return *this;
        }

        /// @brief Set the help text description.
        OptionDef &set_description(const std::string &s)
        {
            m_description = s;
            return *this;
        }

        /// @brief Set the help text description (move).
        OptionDef &set_description(std::string &&s)
        {
            m_description = std::move(s);
            return *this;
        }

        /// @brief Set whether this option consumes a value token.
        OptionDef &set_has_value(bool v)
        {
            m_has_value = v;
            return *this;
        }

        /// @brief Set the compile-time hash for ParseContext indexing.
        OptionDef &set_key_hash(size_t h)
        {
            m_key_hash = h;
            return *this;
        }

        /// @brief Set the runtime type tag.
        OptionDef &set_value_tag(ValueTag t)
        {
            m_value_tag = t;
            return *this;
        }

        /// @brief Register the default value string.
        OptionDef &set_default_str(std::string s)
        {
            m_default_str = decltype(m_default_str)::Some(std::move(s));
            return *this;
        }

        // ── Fluent chaining ──

        /// @brief Mark this option as required (must appear on command line).
        OptionDef &required(bool r = true)
        {
            m_required = r;
            return *this;
        }

        /// @brief Register a completer function for shell completion.
        OptionDef &completer(std::function<std::vector<std::string>()> fn)
        {
            m_completer = std::move(fn);
            return *this;
        }

        /// @brief Get the registered completer function.
        const std::function<std::vector<std::string>()> &completer_fn() const noexcept
        {
            return m_completer;
        }

    private:
        std::string m_long_name;
        char m_short_name{};
        std::string m_description;
        bool m_has_value{};
        bool m_required{};
        size_t m_key_hash{};
        ValueTag m_value_tag{};
        pjh::result::Option<std::string> m_default_str;

        std::function<std::vector<std::string>()> m_completer;
    };

    /// @brief Proxy returned by option() when a default is provided.
    ///
    /// Forwards all getter/setter calls to the underlying OptionDef.
    /// Excludes .required() to prevent default + required contradiction at compile time.
    class OptionDefWithDefault
    {
        OptionDef &m_def;

    public:
        explicit OptionDefWithDefault(OptionDef &def) noexcept : m_def(def) {}

        // ── Getters ──

        /// @brief Get the long option name (delegated).
        const std::string &long_name() const noexcept { return m_def.long_name(); }

        /// @brief Get the short option character (delegated).
        char short_name() const noexcept { return m_def.short_name(); }

        /// @brief Get the help text description (delegated).
        const std::string &description() const noexcept { return m_def.description(); }

        /// @brief Whether the option consumes a value (delegated).
        bool has_value() const noexcept { return m_def.has_value(); }

        /// @brief Whether the option is required (always false for default-wrapped
        /// options).
        bool is_required() const noexcept { return m_def.is_required(); }

        /// @brief Get the compile-time hash (delegated).
        size_t key_hash() const noexcept { return m_def.key_hash(); }

        /// @brief Get the runtime type tag (delegated).
        ValueTag value_tag() const noexcept { return m_def.value_tag(); }

        /// @brief Whether a default value exists (delegated).
        bool has_default() const noexcept { return m_def.has_default(); }

        /// @brief Get the default value string (delegated).
        const std::string &default_str() const noexcept { return m_def.default_str(); }

        // ── Setters (forward all except required()) ──

        /// @brief Set the long option name (delegated).
        OptionDefWithDefault &set_long_name(const std::string &s)
        {
            m_def.set_long_name(s);
            return *this;
        }

        /// @brief Set the long option name, move (delegated).
        OptionDefWithDefault &set_long_name(std::string &&s)
        {
            m_def.set_long_name(std::move(s));
            return *this;
        }

        /// @brief Set the short option character (delegated).
        OptionDefWithDefault &set_short_name(char c)
        {
            m_def.set_short_name(c);
            return *this;
        }

        /// @brief Set the help text description (delegated).
        OptionDefWithDefault &set_description(const std::string &s)
        {
            m_def.set_description(s);
            return *this;
        }

        /// @brief Set the help text description, move (delegated).
        OptionDefWithDefault &set_description(std::string &&s)
        {
            m_def.set_description(std::move(s));
            return *this;
        }

        /// @brief Set whether the option consumes a value (delegated).
        OptionDefWithDefault &set_has_value(bool v)
        {
            m_def.set_has_value(v);
            return *this;
        }

        /// @brief Set the compile-time hash (delegated).
        OptionDefWithDefault &set_key_hash(size_t h)
        {
            m_def.set_key_hash(h);
            return *this;
        }

        /// @brief Set the runtime type tag (delegated).
        OptionDefWithDefault &set_value_tag(ValueTag t)
        {
            m_def.set_value_tag(t);
            return *this;
        }

        /// @brief Register the default value string (delegated).
        OptionDefWithDefault &set_default_str(std::string s)
        {
            m_def.set_default_str(std::move(s));
            return *this;
        }

        // ── Fluent chaining (only completer, NOT required) ──

        /// @brief Register a completer function (delegated).
        OptionDefWithDefault &completer(std::function<std::vector<std::string>()> fn)
        {
            m_def.completer(std::move(fn));
            return *this;
        }

        /// @brief Get the registered completer function (delegated).
        const std::function<std::vector<std::string>()> &completer_fn() const noexcept
        {
            return m_def.completer_fn();
        }
    };

}  // namespace pjh::cli

#endif  // INCLUDE_PJH_CLI_OPTION_DEF_HPP