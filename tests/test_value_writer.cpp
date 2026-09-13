#include <doctest/doctest.h>

#include <filesystem>
#include <pjh_cli/command/leaf_command.hpp>
#include <pjh_cli/core/fixed_string.hpp>
#include <pjh_cli/core/type.hpp>
#include <pjh_cli/parse/parse_context.hpp>
#include <pjh_cli/parse/parser.hpp>
#include <pjh_cli/parse/value_writer.hpp>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

using namespace pjh::cli;

TEST_CASE("ValueWriter stores every real ValueTag")
{
    ParseContext ctx;

    REQUIRE(ValueWriter::apply_arg_value(
                ctx, key_hash(fixed_string("b")), ValueTag::Bool, "true")
                .is_ok());
    REQUIRE(ValueWriter::apply_arg_value(
                ctx, key_hash(fixed_string("i")), ValueTag::Int, "42")
                .is_ok());
    REQUIRE(ValueWriter::apply_arg_value(
                ctx, key_hash(fixed_string("d")), ValueTag::Double, "2.5")
                .is_ok());
    REQUIRE(ValueWriter::apply_arg_value(
                ctx, key_hash(fixed_string("s")), ValueTag::String, "hi")
                .is_ok());
    REQUIRE(ValueWriter::apply_arg_value(
                ctx, key_hash(fixed_string("p")), ValueTag::Path, "/tmp/x")
                .is_ok());

    CHECK(ctx.get<bool, fixed_string("b")>() == true);
    CHECK(ctx.get<int, fixed_string("i")>() == 42);
    CHECK(ctx.get<double, fixed_string("d")>() == doctest::Approx(2.5));
    CHECK(ctx.get<std::string, fixed_string("s")>() == "hi");
    CHECK(
        ctx.get<std::filesystem::path, fixed_string("p")>() ==
        std::filesystem::path("/tmp/x"));
}

TEST_CASE("ValueWriter rejects an out-of-range ValueTag")
{
    // ValueTag::Count is the sentinel; task 40 pins it == BuiltinTypes size.
    static_assert(
        static_cast<size_t>(ValueTag::Count) == std::tuple_size_v<detail::BuiltinTypes>);

    ParseContext ctx;
    CHECK_THROWS_AS(
        (void)ValueWriter::apply_arg_value(
            ctx, 0, static_cast<ValueTag>(ValueTag::Count), "x", "arg"),
        LogicError);
    CHECK_THROWS_AS(
        (void)ValueWriter::apply_arg_value(
            ctx, 1, static_cast<ValueTag>(255), "x", "arg"),
        LogicError);
    // The failed writes left no value behind (no silent partial write).
    CHECK_FALSE(ctx.has<0>());
    CHECK_FALSE(ctx.has<1>());
}

TEST_CASE("Parser rejects a positional arg with an unknown ValueTag")
{
    LeafCommand root("test", "Bogus tag");
    root.arg<std::string, 0>("file", "File").m_value_tag = static_cast<ValueTag>(255);
    std::vector<std::string_view> args{"data.txt"};
    CHECK_THROWS_AS((void)Parser::parse_command(root, args, 0), LogicError);
}
