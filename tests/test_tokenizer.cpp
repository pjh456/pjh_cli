#include <doctest/doctest.h>

#include <ostream>
#include <pjh_cli/detail/tokenizer.hpp>
#include <string>
#include <vector>

using namespace pjh::cli::detail;

namespace
{
    /// @brief Join tokens with '|' so failures render readably.
    std::string dump(const std::vector<std::string> &tokens)
    {
        std::string out;
        for (std::size_t i = 0; i < tokens.size(); ++i)
        {
            if (i != 0)
                out += '|';
            out += tokens[i];
        }
        return out;
    }
}  // namespace

TEST_CASE("Tokenizer splits on spaces")
{
    CHECK(dump(Tokenizer::tokenize("a b c")) == "a|b|c");
}

TEST_CASE("Tokenizer splits on tabs")
{
    CHECK(dump(Tokenizer::tokenize("a\tb\tc")) == "a|b|c");
    CHECK(dump(Tokenizer::tokenize("run\t--verbose")) == "run|--verbose");
}

TEST_CASE("Tokenizer treats mixed whitespace as separators")
{
    CHECK(dump(Tokenizer::tokenize("a \t b")) == "a|b");
    CHECK(dump(Tokenizer::tokenize("\t a\t\tb \t")) == "a|b");
}

TEST_CASE("Tokenizer ignores leading and trailing whitespace")
{
    CHECK(dump(Tokenizer::tokenize("  a  ")) == "a");
    CHECK(dump(Tokenizer::tokenize("\t a \t")) == "a");
}

TEST_CASE("Tokenizer returns no tokens for blank input")
{
    CHECK(dump(Tokenizer::tokenize("")) == "");
    CHECK(dump(Tokenizer::tokenize(" \t ")) == "");
}

TEST_CASE("Tokenizer keeps quoted spaces in one token")
{
    CHECK(dump(Tokenizer::tokenize(R"(cmd "my file.txt")")) == "cmd|my file.txt");
}

TEST_CASE("Tokenizer keeps tabs inside quotes")
{
    CHECK(dump(Tokenizer::tokenize("\"a\tb\"")) == "a\tb");
}

TEST_CASE("Tokenizer concatenates quoted and unquoted spans")
{
    CHECK(dump(Tokenizer::tokenize(R"(ab"cd ef"gh)")) == "abcd efgh");
}

TEST_CASE("Tokenizer preserves an empty quoted token")
{
    auto tokens = Tokenizer::tokenize(R"(a "" b)");
    REQUIRE(tokens.size() == 3);
    CHECK(tokens[0] == "a");
    CHECK(tokens[1].empty());
    CHECK(tokens[2] == "b");
}

TEST_CASE("Tokenizer empty quoted token alone")
{
    auto tokens = Tokenizer::tokenize(R"("")");
    REQUIRE(tokens.size() == 1);
    CHECK(tokens[0].empty());
}

TEST_CASE("Tokenizer escapes a quote inside quotes")
{
    CHECK(dump(Tokenizer::tokenize("\"a\\\"b\"")) == "a\"b");
}

TEST_CASE("Tokenizer escapes a quote outside quotes")
{
    CHECK(dump(Tokenizer::tokenize("a\\\"b")) == "a\"b");
}

TEST_CASE("Tokenizer escapes a backslash")
{
    CHECK(dump(Tokenizer::tokenize("\"a\\\\b\"")) == "a\\b");
}

TEST_CASE("Tokenizer escapes a space outside quotes")
{
    CHECK(dump(Tokenizer::tokenize(R"(a\ b)")) == "a b");
}

TEST_CASE("Tokenizer escapes a tab outside quotes")
{
    CHECK(dump(Tokenizer::tokenize("a\\\tb")) == "a\tb");
}

TEST_CASE("Tokenizer keeps Windows path backslashes")
{
    CHECK(dump(Tokenizer::tokenize("\"C:\\Users\\name\"")) == "C:\\Users\\name");
    CHECK(dump(Tokenizer::tokenize(R"(C:\temp)")) == "C:\\temp");
    CHECK(dump(Tokenizer::tokenize(R"(\d+)")) == "\\d+");
}

TEST_CASE("Tokenizer consumes an unterminated quote to end of input")
{
    CHECK(dump(Tokenizer::tokenize(R"(a "b c)")) == "a|b c");
}

TEST_CASE("Tokenizer keeps a trailing backslash")
{
    CHECK(dump(Tokenizer::tokenize(R"(a\)")) == "a\\");
    CHECK(dump(Tokenizer::tokenize("\\")) == "\\");
}

TEST_CASE("Tokenizer is_separator classifies space and tab only")
{
    CHECK(Tokenizer::is_separator(' '));
    CHECK(Tokenizer::is_separator('\t'));
    CHECK_FALSE(Tokenizer::is_separator('\n'));
    CHECK_FALSE(Tokenizer::is_separator('a'));
}

TEST_CASE("Tokenizer parse_long_option still splits name and value")
{
    auto port = Tokenizer::parse_long_option("--port=8080");
    CHECK(port.name == "port");
    CHECK(port.value == "8080");
    CHECK(port.has_equals);
    CHECK_FALSE(port.is_negation);

    auto no_color = Tokenizer::parse_long_option("--no-color");
    CHECK(no_color.name == "no-color");
    CHECK(no_color.value.empty());
    CHECK_FALSE(no_color.has_equals);
    CHECK(no_color.is_negation);
    CHECK(no_color.negated_name == "color");
}
