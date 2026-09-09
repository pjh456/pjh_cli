#ifndef INCLUDE_PJH_CLI_OPTION_DEF_HPP
#define INCLUDE_PJH_CLI_OPTION_DEF_HPP

#include <cstddef>
#include <functional>
#include <pjh_cli/core/error.hpp>
#include <pjh_cli/core/type.hpp>
#include <pjh_cli/detail/concept.hpp>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace pjh::cli
{
    class BaseCommand;  // for OptionDef's return types

    // ── Forward declarations for typed subclasses ──

    /// @cond FORWARD_DECLS
    class IntOption;
    class CountOption;
    class BoolOption;
    class StrOption;
    class FloatOption;
    class PathOption;
    template <typename E>
        requires std::is_enum_v<E>
    class EnumOption;
    /// @endcond

    // ──────────────────────────────────────────
    //  OptionDef — base class
    // ──────────────────────────────────────────

    /// @brief Base class for all option definitions.
    ///
    /// Holds common fields (name, description, key hash, required flag, etc.)
    /// and provides virtual `parse_value()` and `default_option_value()` that
    /// each typed subclass overrides.  Options are stored polymorphically in
    /// \`BaseCommand::m_options\` as \`unique_ptr<OptionDef>\`.
    ///
    /// The option layer only yields validated values; the parse layer
    /// (ValueWriter) is the single writer of ParseContext storage.
    class OptionDef
    {
    public:
        OptionDef() = default;
        virtual ~OptionDef() = default;

        OptionDef(const OptionDef &) = delete;
        OptionDef &operator=(const OptionDef &) = delete;
        OptionDef(OptionDef &&) = delete;
        OptionDef &operator=(OptionDef &&) = delete;

        // ── Getters ──

        /// @brief Long option name (e.g. "verbose").
        const std::string &long_name() const noexcept { return m_long_name; }

        /// @brief Canonical long-option display used in error messages.
        /// @return "--" + long_name() (e.g. "--verbose").
        std::string display_name() const { return "--" + m_long_name; }

        /// @brief Short option character (0 if none).
        char short_name() const noexcept { return m_short_name; }

        /// @brief Help text description.
        const std::string &description() const noexcept { return m_description; }

        /// @brief Whether this option consumes a value token.
        virtual bool has_value() const noexcept { return m_has_value; }

        /// @brief Whether this option must appear on the command line.
        ///        Default false. Overridden by WithRequired mixin.
        virtual bool is_required() const noexcept { return false; }

        /// @brief Compile-time hash used to index ParseContext.
        size_t key_hash() const noexcept { return m_key_hash; }

        /// @brief Runtime type tag for value conversion.
        ValueTag value_tag() const noexcept { return m_value_tag; }

        /// @brief Registered completer callback (empty if none).
        const std::function<std::vector<std::string>()> &completer_fn() const noexcept
        {
            return m_completer;
        }

        /// @brief Whether a typed default value has been registered.
        virtual bool has_default() const noexcept { return false; }

        /// @brief Human-readable default value string for help output.
        ///        Returns empty string if there is no default.
        virtual std::string default_value_str() const { return ""; }

        /// @brief Whether this option counts occurrences (-vvv → 3).
        virtual bool is_counting() const noexcept { return false; }

        /// @brief Whether this option supports --no-xxx negation.
        virtual bool is_negatable() const noexcept { return false; }

        /// @brief Whether this option accepts repeated values (--opt a --opt b).
        ///        When true, also enables multi-value consumption (--opt a b c).
        ///        Default false. Overridden by WithRepeatable mixin.
        virtual bool is_repeatable() const noexcept { return false; }

        /// @brief Environment variable name (empty if none).
        ///        Default empty. Overridden by WithEnv mixin.
        virtual const std::string &env_var() const noexcept
        {
            static const std::string empty;
            return empty;
        }

        /// @brief Run the convert+validate pipeline and yield a type-erased
        ///        value without touching ParseContext.
        /// @param raw The raw string value from the command line / env /
        ///        default.
        /// @return Ok(OptionValue) on success, or a conversion/validation
        ///         CliError.
        virtual CliResult<OptionValue> parse_value(std::string_view raw) const;

        /// @brief Yield the validated default value, if one is registered.
        /// @return Ok(Some(value)) when a default exists and validates,
        ///         Ok(None()) when no default is registered, or Err when it
        ///         fails validation.
        virtual CliResult<pjh::result::Option<OptionValue>> default_option_value() const;

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
        size_t m_key_hash{};
        ValueTag m_value_tag{};
        std::function<std::vector<std::string>()> m_completer;
    };

    // ── Virtual method default implementations ──

    /// @brief Default: option does not accept a value; returns an error.
    inline CliResult<OptionValue> OptionDef::parse_value(std::string_view) const
    {
        return CliFailure{ErrorFactory::option_does_not_accept_value(display_name())};
    }

    /// @brief Default: no default value registered.
    inline CliResult<pjh::result::Option<OptionValue>> OptionDef::default_option_value()
        const
    {
        return CliResult<pjh::result::Option<OptionValue>>::Ok(
            pjh::result::Option<OptionValue>::None());
    }

}  // namespace pjh::cli

#endif  // INCLUDE_PJH_CLI_OPTION_DEF_HPP
