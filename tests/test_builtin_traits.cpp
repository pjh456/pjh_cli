#include <doctest/doctest.h>

#include <filesystem>
#include <pjh_cli/core/converter.hpp>
#include <pjh_cli/core/type.hpp>
#include <string>
#include <string_view>
#include <tuple>

using namespace pjh::cli;

TEST_CASE("BuiltinTraits covers exactly the five builtin types")
{
    static_assert(detail::BuiltinType<bool>);
    static_assert(detail::BuiltinType<int>);
    static_assert(detail::BuiltinType<double>);
    static_assert(detail::BuiltinType<std::string>);
    static_assert(detail::BuiltinType<std::filesystem::path>);

    static_assert(!detail::BuiltinType<float>);
    static_assert(!detail::BuiltinType<long>);
    static_assert(!detail::BuiltinType<unsigned>);
    static_assert(!detail::BuiltinType<char>);
    static_assert(!detail::BuiltinType<std::string_view>);

    CHECK(detail::BuiltinTraits<bool>::is_builtin);
    CHECK_FALSE(detail::BuiltinTraits<float>::is_builtin);
}

TEST_CASE("value_tag_v matches the ValueTag enumerators")
{
    static_assert(detail::value_tag_v<bool> == ValueTag::Bool);
    static_assert(detail::value_tag_v<int> == ValueTag::Int);
    static_assert(detail::value_tag_v<double> == ValueTag::Double);
    static_assert(detail::value_tag_v<std::string> == ValueTag::String);
    static_assert(detail::value_tag_v<std::filesystem::path> == ValueTag::Path);

    CHECK(detail::value_tag_v<int> == ValueTag::Int);
}

TEST_CASE("type_index_v is stable and ordered")
{
    static_assert(detail::type_index_v<bool> == 0);
    static_assert(detail::type_index_v<int> == 1);
    static_assert(detail::type_index_v<double> == 2);
    static_assert(detail::type_index_v<std::string> == 3);
    static_assert(detail::type_index_v<std::filesystem::path> == 4);

    CHECK(detail::type_index_v<double> == 2);
}

TEST_CASE("BuiltinTypes order matches tag ordinals")
{
    CHECK(detail::builtin_tags_match_order(static_cast<detail::BuiltinTypes *>(nullptr)));
    static_assert(
        static_cast<size_t>(ValueTag::Count) == std::tuple_size_v<detail::BuiltinTypes>);
    CHECK(
        static_cast<size_t>(ValueTag::Count) == std::tuple_size_v<detail::BuiltinTypes>);
}

TEST_CASE("BuiltinTraits hint labels")
{
    CHECK(detail::BuiltinTraits<bool>::hint_name == "BOOL");
    CHECK(detail::BuiltinTraits<int>::hint_name == "INT");
    CHECK(detail::BuiltinTraits<double>::hint_name == "FLOAT");
    CHECK(detail::BuiltinTraits<std::string>::hint_name == "STR");
    CHECK(detail::BuiltinTraits<std::filesystem::path>::hint_name == "PATH");

    auto names = detail::hint_names(static_cast<detail::BuiltinTypes *>(nullptr));
    CHECK(names[static_cast<size_t>(ValueTag::Bool)] == "BOOL");
    CHECK(names[static_cast<size_t>(ValueTag::Int)] == "INT");
    CHECK(names[static_cast<size_t>(ValueTag::Double)] == "FLOAT");
    CHECK(names[static_cast<size_t>(ValueTag::String)] == "STR");
    CHECK(names[static_cast<size_t>(ValueTag::Path)] == "PATH");
}

TEST_CASE("BuiltinTraits default_string")
{
    CHECK(detail::BuiltinTraits<bool>::default_string(true) == "true");
    CHECK(detail::BuiltinTraits<bool>::default_string(false) == "false");
    CHECK(detail::BuiltinTraits<int>::default_string(42) == "42");
    CHECK(detail::BuiltinTraits<double>::default_string(2.5) == "2.500000");
    CHECK(detail::BuiltinTraits<std::string>::default_string("hello") == "hello");
    CHECK(
        detail::BuiltinTraits<std::filesystem::path>::default_string(
            std::filesystem::path("a/b")) == "a/b");
}
