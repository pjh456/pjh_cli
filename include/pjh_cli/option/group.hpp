#ifndef INCLUDE_PJH_CLI_OPTION_GROUP_HPP
#define INCLUDE_PJH_CLI_OPTION_GROUP_HPP

#include <cstddef>
#include <string>
#include <vector>

namespace pjh::cli
{

    /// @brief Constraint mode for option groups.
    enum class GroupMode : unsigned
    {
        ExactlyOne,  ///< Exactly one option in the group must be set.
        AtMostOne,   ///< Zero or one option in the group may be set.
        AtLeastOne,  ///< At least one option in the group must be set.
    };

    /// @brief A group of options with a mutual-exclusion or requirement constraint.
    struct OptionGroup
    {
        std::vector<size_t> key_hashes;  ///< Compile-time key hashes of group members.
        std::vector<std::string>
            option_names;  ///< Long option names (for error messages).
        GroupMode mode;    ///< Constraint type.
    };

}  // namespace pjh::cli

#endif  // INCLUDE_PJH_CLI_OPTION_GROUP_HPP
