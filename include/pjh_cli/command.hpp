#ifndef INCLUDE_PJH_CLI_COMMAND_HPP
#define INCLUDE_PJH_CLI_COMMAND_HPP

#include "arg_def.hpp"
#include "converter.hpp"
#include "detail/concept.hpp"
#include "option_def.hpp"
#include "parse_context.hpp"
#include "type.hpp"

#include <functional>
#include <list>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace pjh::cli
{
    namespace detail
    {
        struct transparent_string_hash
        {
            using is_transparent = void;
            size_t operator()(std::string_view sv) const noexcept
            {
                return std::hash<std::string_view>{}(sv);
            }
        };
    }

    /// @brief Bitmask flags controlling where a Command appears.
    ///
    /// Combine with `|`:  `set_visibility(Visibility::Cli | Visibility::Repl)`
    enum class Visibility : unsigned
    {
        Hidden = 0, ///< Hidden from help / completion everywhere
        Repl = 1,   ///< Visible in interactive REPL
        Cli = 2,    ///< Visible in batch CLI
        Both = 3,   ///< Visible everywhere (default)
    };

    /// @brief Bitwise OR for Visibility flags.
    constexpr Visibility
    operator|(
        Visibility a,
        Visibility b) noexcept
    {
        return static_cast<Visibility>(
            static_cast<unsigned>(a) |
            static_cast<unsigned>(b));
    }

    /// @brief Bitwise AND for Visibility flags.
    constexpr Visibility
    operator&(
        Visibility a,
        Visibility b) noexcept
    {
        return static_cast<Visibility>(
            static_cast<unsigned>(a) &
            static_cast<unsigned>(b));
    }

    /// @brief Policy for handling extra positional arguments beyond registered arg<N>.
    enum class ExtraArgsPolicy : unsigned
    {
        Ignore, ///< Silently discard (default, POSIX convention).
        Error,  ///< Return CliError on any extra positional argument.
        Store,  ///< Append to ParseContext::extra_args() for runtime inspection.
    };

    /// @brief Composite node in the command tree.
    ///
    /// Every Command can hold:
    ///   - Named options (--flag -o) via option<T, Key>()
    ///   - Positional arguments via arg<T, Index>()
    ///   - Child subcommands via add_command()
    ///   - An action callback to execute when matched
    ///
    /// App inherits from Command and represents the root node.
    class Command
    {
    public:
        /// @brief Construct a command with optional name and description.
        ///        The root command (App) typically has name set to the program name.
        explicit Command(
            std::string name = "",
            std::string description = "");

        virtual ~Command() = default;

        Command(const Command &) = delete;
        Command &operator=(const Command &) = delete;
        Command(Command &&) = delete;
        Command &operator=(Command &&) = delete;

        // ──────────────────────────────────────────
        //  Registration
        // ──────────────────────────────────────────

        /// @brief Register a named option (no short name, no default).
        /// @tparam T Value type (bool → flag, others → value-consuming).
        /// @tparam Key Compile-time identifier (fixed_string).
        template <typename T, auto Key>
            requires detail::Parseable<T>
        OptionDef &
        option(
            std::string long_name,
            std::string description);

        /// @brief Register a named option with a default value (no short name).
        /// @note Returns OptionDefWithDefault — .required() is NOT available,
        ///       because default + required is a contradiction.
        template <typename T, auto Key>
            requires detail::Parseable<T>
        OptionDefWithDefault
        option(
            std::string long_name,
            std::string description,
            T default_value);

        /// @brief Register a named option with short name (no default).
        template <typename T, auto Key>
            requires detail::Parseable<T>
        OptionDef &
        option(
            std::string long_name,
            char short_name,
            std::string description);

        /// @brief Register a named option with short name and a default value.
        /// @note Returns OptionDefWithDefault — .required() is NOT available,
        ///       because default + required is a contradiction.
        template <typename T, auto Key>
            requires detail::Parseable<T>
        OptionDefWithDefault
        option(
            std::string long_name,
            char short_name,
            std::string description,
            T default_value);

        /// @brief Register a positional argument identified by compile-time Index.
        /// @tparam T Value type.
        /// @tparam Index Positional index (0, 1, 2, ...).
        template <typename T, size_t Index>
            requires detail::Parseable<T>
        ArgDef &
        arg(
            std::string name,
            std::string description);

        /// @brief Add a child subcommand. Returns reference to the child.
        Command &
        add_command(
            std::string name,
            std::string description);

        /// @brief Set the action callback invoked when this command is matched.
        Command &
        action(
            std::function<
                CliResult<void>(ParseContext &)>
                fn);

        /// @brief Set the runtime enable predicate.
        ///        A disabled command is treated as non-existent during matching.
        Command &
        enabled(std::function<bool()> pred);

        /// @brief Set visibility level (affects help / REPL matching).
        Command &
        set_visibility(Visibility v);

        /// @brief Set extra positional args handling policy (default: Ignore).
        Command &
        set_extra_args(ExtraArgsPolicy p);

        // ──────────────────────────────────────────
        //  Queries
        // ──────────────────────────────────────────

        const std::string &
        name()
            const noexcept { return m_name; }

        const std::string &
        description()
            const noexcept { return m_description; }

        Visibility
        visibility()
            const noexcept { return m_visibility; }

        /// @brief Evaluate the enabled predicate.
        bool
        is_enabled()
            const { return m_enabled(); }

        Command *
        parent()
            const noexcept { return m_parent; }

        const std::list<Command> &
        subcommands()
            const noexcept { return m_subcommands; }

        const std::vector<OptionDef> &
        options()
            const noexcept { return m_options; }

        const std::vector<ArgDef> &
        args()
            const noexcept { return m_args; }

        ExtraArgsPolicy
        extra_args_policy()
            const noexcept { return m_extra_args_policy; }

        /// @brief Find a direct child subcommand by exact name match.
        Command *
        find_subcommand(
            std::string_view name) noexcept;

        /// @brief Const overload.
        const Command *
        find_subcommand(
            std::string_view name)
            const noexcept;

        /// @brief Look up an option by its long name (without -- prefix).
        const OptionDef *
        find_option_by_long(
            std::string_view name)
            const noexcept;

        /// @brief Look up an option by its short character.
        const OptionDef *
        find_option_by_short(char c)
            const noexcept;

        // ──────────────────────────────────────────
        //  Parser helpers
        // ──────────────────────────────────────────

        /// @brief Create an empty ParseContext for this command.
        ParseContext
        create_context() const noexcept;

        /// @brief Pre-fill context with default values from registered options.
        CliResult<void>
        apply_defaults(
            ParseContext &ctx)
            const;

        /// @brief Execute the registered action callback.
        CliResult<void>
        execute(ParseContext &ctx) const;

    private:
        std::string m_name;
        std::string m_description;
        Command *m_parent = nullptr;

        std::vector<OptionDef> m_options;
        std::vector<ArgDef> m_args;
        std::list<Command> m_subcommands;

        std::unordered_map<
            std::string, size_t,
            detail::transparent_string_hash,
            std::equal_to<void>>
            m_option_by_long;
        std::unordered_map<
            char,
            size_t>
            m_option_by_short;

        ExtraArgsPolicy m_extra_args_policy =
            ExtraArgsPolicy::Ignore;
        Visibility m_visibility =
            Visibility::Both;
        std::function<bool()> m_enabled =
            []
        { return true; };
        std::function<
            CliResult<void>(ParseContext &)>
            m_action;
    };

    // ──────────────────────────────────────────────
    //  Template implementations
    // ──────────────────────────────────────────────

    template <typename T, auto Key>
        requires detail::Parseable<T>
    OptionDef &
    Command::option(
        std::string long_name,
        std::string description)
    {
        return option<T, Key>(
            std::move(long_name),
            0,
            std::move(description));
    }

    template <typename T, auto Key>
        requires detail::Parseable<T>
    OptionDefWithDefault
    Command::option(
        std::string long_name,
        std::string description,
        T default_value)
    {
        auto &def = option<T, Key>(
            std::move(long_name),
            0,
            std::move(description));
        def.m_apply_default =
            [h = key_hash(Key),
             v = std::move(default_value)](
                ParseContext &ctx) mutable
        {
            ctx.set_value<T>(h, std::move(v));
        };
        return OptionDefWithDefault(def);
    }

    template <typename T, auto Key>
        requires detail::Parseable<T>
    OptionDef &
    Command::option(
        std::string long_name,
        char short_name,
        std::string description)
    {
        // Strip leading "--" if present
        if (long_name.size() > 2 &&
            long_name[0] == '-' &&
            long_name[1] == '-')
            long_name = long_name.substr(2);

        constexpr size_t h = key_hash(Key);

        auto &def = m_options.emplace_back();
        def.m_long_name = std::move(long_name);
        def.m_short_name = short_name;
        def.m_description = std::move(description);
        def.m_has_value = !detail::Flag<T>;
        def.m_key_hash = h;
        def.m_apply =
            [h](ParseContext &ctx,
                std::string_view s)
            -> CliResult<void>
        {
            auto r = Converter<T>::from_string(s);
            if (r.is_err())
                return CliResult<void>::Err(
                    std::move(r).unwrap_err());
            ctx.set_value<T>(h, r.unwrap());
            return CliResult<void>::Ok();
        };

        m_option_by_long[def.m_long_name] =
            m_options.size() - 1;
        if (short_name != 0)
            m_option_by_short[short_name] =
                m_options.size() - 1;

        return def;
    }

    template <typename T, auto Key>
        requires detail::Parseable<T>
    OptionDefWithDefault
    Command::option(
        std::string long_name,
        char short_name,
        std::string description,
        T default_value)
    {
        auto &def = option<T, Key>(
            std::move(long_name),
            short_name,
            std::move(description));
        def.m_apply_default =
            [h = key_hash(Key),
             v = std::move(default_value)](
                ParseContext &ctx) mutable
        {
            ctx.set_value<T>(h, std::move(v));
        };
        return OptionDefWithDefault(def);
    }

    template <typename T, size_t Index>
        requires detail::Parseable<T>
    ArgDef &
    Command::arg(
        std::string name,
        std::string description)
    {
        constexpr size_t h = key_hash(Index);

        auto &def = m_args.emplace_back();
        def.m_name = std::move(name);
        def.m_description = std::move(description);
        def.m_index = Index;
        def.m_key_hash = h;
        def.m_apply =
            [h](ParseContext &ctx,
                std::string_view s)
            -> CliResult<void>
        {
            auto r = Converter<T>::from_string(s);
            if (r.is_err())
                return CliResult<void>::Err(
                    std::move(r).unwrap_err());
            ctx.set_value<T>(h, r.unwrap());
            return CliResult<void>::Ok();
        };

        return def;
    }

} // namespace pjh::cli

#endif // INCLUDE_PJH_CLI_COMMAND_HPP
