#ifndef INCLUDE_PJH_CLI_OPTION_DEF_HPP
#define INCLUDE_PJH_CLI_OPTION_DEF_HPP

#include <functional>
#include <memory>
#include <pjh_result.hpp>
#include <string>
#include <string_view>
#include <vector>

#include "fixed_string.hpp"
#include "type.hpp"

namespace pjh::cli
{
    class Command;
    class ParseContext;

    // ──────────────────────────────────────────
    //  OptionDef — base class
    // ──────────────────────────────────────────

    /// @brief Base class for all option definitions.
    ///
    /// Holds common fields (name, description, key hash, value tag, required
    /// flag, default string, completer).  Subclasses (IntOption, BoolOption,
    /// etc.) add type-specific chainable methods.  The parser uses the base
    /// class interface exclusively.
    class OptionDef
    {
    public:
        /// @brief Default constructor; default_str is None.
        OptionDef() : m_default_str(pjh::result::Option<std::string>::None()) {}

        /// @brief Virtual destructor for polymorphic storage.
        virtual ~OptionDef() = default;

        OptionDef(const OptionDef &) = delete;
        OptionDef &operator=(const OptionDef &) = delete;
        OptionDef(OptionDef &&) = delete;
        OptionDef &operator=(OptionDef &&) = delete;

        // ── Getters ──

        /// @brief Long option name (e.g. "verbose").
        const std::string &long_name() const noexcept { return m_long_name; }

        /// @brief Short option character (0 if none).
        char short_name() const noexcept { return m_short_name; }

        /// @brief Help text description.
        const std::string &description() const noexcept { return m_description; }

        /// @brief Whether this option consumes a value token.
        bool has_value() const noexcept { return m_has_value; }

        /// @brief Whether this option must appear on the command line.
        bool is_required() const noexcept { return m_required; }

        /// @brief Compile-time hash used to index ParseContext.
        size_t key_hash() const noexcept { return m_key_hash; }

        /// @brief Runtime type tag for value conversion.
        ValueTag value_tag() const noexcept { return m_value_tag; }

        /// @brief Whether a default value has been registered.
        bool has_default() const noexcept { return m_default_str.is_some(); }

        /// @brief Default value string (empty if no default).
        const std::string &default_str() const noexcept { return m_default_str.unwrap(); }

        /// @brief Registered completer callback (empty if none).
        const std::function<std::vector<std::string>()> &completer_fn() const noexcept
        {
            return m_completer;
        }

        // ── Chainable setters ──

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

        /// @brief Mark this option as required.
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

    protected:
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

    // ──────────────────────────────────────────
    //  Typed option subclasses
    // ──────────────────────────────────────────

    /// @brief Integer-valued option. Future chain: .min(), .max(), .count()
    class IntOption : public OptionDef
    {
    };

    /// @brief Boolean flag option. Future chain: .negatable()
    class BoolOption : public OptionDef
    {
    };

    /// @brief String-valued option. Future chain: .choices(), .repeatable()
    class StrOption : public OptionDef
    {
    };

    /// @brief Double-valued floating-point option.
    class FloatOption : public OptionDef
    {
    };

    /// @brief Filesystem path option.
    class PathOption : public OptionDef
    {
    };

    // ──────────────────────────────────────────
    //  OptionBuilder — chainable registration API
    //  (declaration only; definitions in command.hpp)
    // ──────────────────────────────────────────

    /// @brief Builder returned by Command::option<Key>().
    ///
    /// Holds common registration fields and provides type-dispatch methods
    /// (.integer(), .boolean(), .str(), …) that create the corresponding
    /// typed subclass and add it to the command.
    /// @tparam Key Compile-time fixed_string identifier.
    template <auto Key>
        requires detail::OptionKey<decltype(Key)>
    class OptionBuilder
    {
        Command &m_cmd;
        std::string m_long_name;
        std::string m_description;
        char m_short_name = 0;

    public:
        /// @brief Construct a builder for the given command and option name.
        OptionBuilder(Command &cmd, std::string long_name, std::string description) :
            m_cmd(cmd),
            m_long_name(std::move(long_name)),
            m_description(std::move(description))
        {
        }

        /// @brief Set the short option character.
        void set_short_name(char c) noexcept { m_short_name = c; }

        /// @brief Create as an integer-valued option.
        /// @return Reference to the newly created IntOption.
        IntOption &integer();

        /// @brief Create as a boolean flag option.
        /// @return Reference to the newly created BoolOption.
        BoolOption &boolean();

        /// @brief Create as a string-valued option (default when no type is
        ///        specified).
        /// @return Reference to the newly created StrOption.
        StrOption &str();

        /// @brief Create as a double-valued floating-point option.
        /// @return Reference to the newly created FloatOption.
        FloatOption &floating();

        /// @brief Create as a filesystem path option.
        /// @return Reference to the newly created PathOption.
        PathOption &path();
    };

}  // namespace pjh::cli

#endif  // INCLUDE_PJH_CLI_OPTION_DEF_HPP
