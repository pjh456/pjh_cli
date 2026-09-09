#ifndef INCLUDE_PJH_CLI_OPTION_BUILDER_HPP
#define INCLUDE_PJH_CLI_OPTION_BUILDER_HPP

#include <concepts>
#include <filesystem>
#include <pjh_cli/core/error.hpp>
#include <pjh_cli/core/type.hpp>
#include <pjh_cli/detail/concept.hpp>
#include <pjh_cli/option/option_def.hpp>
#include <string>
#include <type_traits>

namespace pjh::cli
{

    class BaseCommand;

    /// @brief Builder returned by BaseCommand::option<Key>().
    ///
    /// Holds common registration fields and provides type-dispatch methods
    /// (.integer(), .boolean(), .str(), ...) that create the corresponding
    /// typed subclass and add it to the command.
    /// @tparam Key Compile-time fixed_string identifier.
    template <auto Key>
        requires detail::OptionKey<decltype(Key)>
    class OptionBuilder
    {
        BaseCommand &m_cmd;
        std::string m_long_name;
        std::string m_description;
        char m_short_name = 0;

    public:
        /// @brief Construct a builder for the given command and option name.
        /// @param cmd        Target command to register the option on.
        /// @param long_name  Long option name (with or without "--" prefix).
        /// @param description Help text description.
        OptionBuilder(BaseCommand &cmd, std::string long_name, std::string description) :
            m_cmd(cmd),
            m_long_name(std::move(long_name)),
            m_description(std::move(description))
        {
        }

        /// @brief Set the short option character.
        /// @param c Single-character short form (e.g. 'v'), or 0 for none.
        void set_short_name(char c) noexcept { m_short_name = c; }

        /// @brief Create the option as an integer-valued type.
        /// @return Reference to the newly created IntOption.
        IntOption &integer();

        /// @brief Create the option as a counting flag (-vvv -> 3).
        /// @return Reference to the newly created CountOption.
        CountOption &count();

        /// @brief Create the option as a boolean flag type.
        /// @return Reference to the newly created BoolOption.
        BoolOption &boolean();

        /// @brief Create the option as a string-valued type.
        /// @return Reference to the newly created StrOption.
        StrOption &str();

        /// @brief Create the option as a double-valued floating-point type.
        /// @return Reference to the newly created FloatOption.
        FloatOption &floating();

        /// @brief Create the option as a filesystem path type.
        /// @return Reference to the newly created PathOption.
        PathOption &path();

        /// @brief Create the option as an enum-valued type.
        /// @tparam E Enum type whose string-to-value mapping is provided
        ///           at registration time via `.mapping()`.
        /// @return Reference to the newly created EnumOption<E>.
        template <typename E>
            requires std::is_enum_v<E>
        EnumOption<E> &enum_type();

    private:
        /// @brief Instantiate Opt, derive its ValueTag from Opt::ValueType and
        ///        register it on the command.
        /// @tparam Opt Concrete option type (must expose ValueType).
        /// @param has_val Whether the option consumes a CLI value token.
        /// @return Reference to the newly created option.
        template <typename Opt>
        Opt &make_option(bool has_val);
    };

}  // namespace pjh::cli

namespace pjh::cli::detail
{

    /// @brief Dispatch an OptionBuilder to the correct typed subclass and
    ///        set a default value.
    ///
    /// Exhaustive by construction: the final dependent `static_assert` makes a
    /// new BuiltinType without a branch a compile-time error at the first
    /// inferred-default call, not undefined behaviour.
    template <typename T, typename Builder>
        requires detail::BuiltinType<T>
    OptionDef &dispatch_default(Builder &builder, T default_value)
    {
        if constexpr (std::same_as<T, bool>)
            return builder.boolean().default_value(default_value);
        else if constexpr (std::same_as<T, int>)
            return builder.integer().default_value(default_value);
        else if constexpr (std::same_as<T, double>)
            return builder.floating().default_value(default_value);
        else if constexpr (std::same_as<T, std::string>)
            return builder.str().default_value(default_value);
        else if constexpr (std::same_as<T, std::filesystem::path>)
            return builder.path().default_value(default_value);
        else
            static_assert(
                always_false_v<T>,
                "unhandled BuiltinType: add a dispatch_default branch");
    }

}  // namespace pjh::cli::detail

#endif  // INCLUDE_PJH_CLI_OPTION_BUILDER_HPP
