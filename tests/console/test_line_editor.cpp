#include <doctest/doctest.h>

#include <pjh_cli/console/in_memory_history.hpp>
#include <pjh_cli/console/line_editor.hpp>
#include <string>
#include <string_view>
#include <vector>

#include "test_helpers.hpp"

using namespace pjh::cli;

namespace
{
    /// @brief Push one Character KeyEvent per byte of @p text.
    void chars(ScriptedTerminal &term, std::string_view text)
    {
        for (char c : text) term.keys.push_back({KeyEvent::Code::Character, c});
    }

    /// @brief Push one Up arrow KeyEvent.
    void press_up(ScriptedTerminal &term)
    {
        term.keys.push_back({KeyEvent::Code::Up, 0});
    }

    /// @brief Push one Down arrow KeyEvent.
    void press_down(ScriptedTerminal &term)
    {
        term.keys.push_back({KeyEvent::Code::Down, 0});
    }

    /// @brief Completion callback that always returns an empty candidate list.
    std::vector<CompletionCandidate> no_candidates(std::string_view, std::size_t)
    {
        return {};
    }

    /// @brief Hint callback that always returns an empty hint.
    std::string no_hint(std::string_view, std::size_t) { return {}; }
}  // namespace

TEST_CASE("LineEditor returns typed line on Enter")
{
    ScriptedTerminal term;
    chars(term, "hello");
    term.keys.push_back({KeyEvent::Code::Enter, 0});

    LineEditor editor(term, "> ");
    std::string line;
    CHECK(editor.read_line(line, no_candidates, no_hint));
    CHECK(line == "hello");
}

TEST_CASE("LineEditor backspace removes character")
{
    ScriptedTerminal term;
    chars(term, "ab");
    term.keys.push_back({KeyEvent::Code::Backspace, 0});
    term.keys.push_back({KeyEvent::Code::Enter, 0});

    LineEditor editor(term, "> ");
    std::string line;
    CHECK(editor.read_line(line, no_candidates, no_hint));
    CHECK(line == "a");
    CHECK(term.written == "> a\n");
}

TEST_CASE("LineEditor Tab completes unique candidate")
{
    ScriptedTerminal term;
    chars(term, "ser");
    term.keys.push_back({KeyEvent::Code::Tab, 0});
    term.keys.push_back({KeyEvent::Code::Enter, 0});

    CompletionFn complete = [](std::string_view line, std::size_t)
    {
        std::vector<CompletionCandidate> out;
        if (line == "ser")
            out.push_back({"serve"});
        return out;
    };

    LineEditor editor(term, "> ");
    std::string line;
    CHECK(editor.read_line(line, complete, no_hint));
    CHECK(line == "serve ");
    CHECK(term.written.find("ve ") != std::string::npos);
}

TEST_CASE("LineEditor Tab lists multiple candidates and hint")
{
    ScriptedTerminal term;
    chars(term, "ser");
    term.keys.push_back({KeyEvent::Code::Tab, 0});
    term.keys.push_back({KeyEvent::Code::Enter, 0});

    CompletionFn complete = [](std::string_view, std::size_t)
    {
        return std::vector<CompletionCandidate>{{"serve"}, {"server"}};
    };
    HintFn hint = [](std::string_view, std::size_t)
    {
        return std::string("<HINT>");
    };

    LineEditor editor(term, "> ");
    std::string line;
    CHECK(editor.read_line(line, complete, hint));
    CHECK(line == "ser");
    CHECK(term.written.find("serve  server") != std::string::npos);
    CHECK(term.written.find("<HINT>") != std::string::npos);
    CHECK(term.written.find("> ser") != std::string::npos);
}

TEST_CASE("LineEditor Tab with no candidates prints hint only")
{
    ScriptedTerminal term;
    chars(term, "zzz");
    term.keys.push_back({KeyEvent::Code::Tab, 0});
    term.keys.push_back({KeyEvent::Code::Enter, 0});

    HintFn hint = [](std::string_view, std::size_t)
    {
        return std::string("no matches");
    };

    LineEditor editor(term, "> ");
    std::string line;
    CHECK(editor.read_line(line, no_candidates, hint));
    CHECK(line == "zzz");
    CHECK(term.written.find("no matches") != std::string::npos);
}

TEST_CASE("LineEditor Ctrl-D ends input")
{
    ScriptedTerminal term;
    chars(term, "partial");
    term.keys.push_back({KeyEvent::Code::Eof, 0});

    LineEditor editor(term, "> ");
    std::string line;
    CHECK_FALSE(editor.read_line(line, no_candidates, no_hint));
}

TEST_CASE("LineEditor Up/Down are no-ops")
{
    ScriptedTerminal term;
    chars(term, "a");
    term.keys.push_back({KeyEvent::Code::Up, 0});
    chars(term, "b");
    term.keys.push_back({KeyEvent::Code::Down, 0});
    term.keys.push_back({KeyEvent::Code::Enter, 0});

    LineEditor editor(term, "> ");
    std::string line;
    CHECK(editor.read_line(line, no_candidates, no_hint));
    CHECK(line == "ab");
}

TEST_CASE("LineEditor echoes prompt once")
{
    ScriptedTerminal term;
    term.keys.push_back({KeyEvent::Code::Enter, 0});

    LineEditor editor(term, "> ");
    std::string line;
    CHECK(editor.read_line(line, no_candidates, no_hint));
    CHECK(term.written.starts_with("> "));
    CHECK(term.written.find("> >") == std::string::npos);
}

TEST_CASE("LineEditor Up recalls most recent history")
{
    InMemoryHistory h;
    h.push("first");
    h.push("second");

    ScriptedTerminal term;
    chars(term, "x");
    press_up(term);
    term.keys.push_back({KeyEvent::Code::Enter, 0});

    LineEditor editor(term, "> ", &h);
    std::string line;
    CHECK(editor.read_line(line, no_candidates, no_hint));
    CHECK(line == "second");
}

TEST_CASE("LineEditor Up Up Up Down navigates history")
{
    InMemoryHistory h;
    h.push("a");
    h.push("b");
    h.push("c");

    ScriptedTerminal term;
    press_up(term);
    press_up(term);
    press_up(term);
    press_down(term);
    term.keys.push_back({KeyEvent::Code::Enter, 0});

    LineEditor editor(term, "> ", &h);
    std::string line;
    CHECK(editor.read_line(line, no_candidates, no_hint));
    CHECK(line == "b");
}

TEST_CASE("LineEditor Down past newest restores draft")
{
    InMemoryHistory h;
    h.push("a");
    h.push("b");

    ScriptedTerminal term;
    chars(term, "x");
    press_up(term);
    press_up(term);
    press_down(term);
    press_down(term);
    term.keys.push_back({KeyEvent::Code::Enter, 0});

    LineEditor editor(term, "> ", &h);
    std::string line;
    CHECK(editor.read_line(line, no_candidates, no_hint));
    CHECK(line == "x");
}

TEST_CASE("LineEditor Up at oldest is no-op")
{
    InMemoryHistory h;
    h.push("only");

    ScriptedTerminal term;
    press_up(term);
    press_up(term);
    term.keys.push_back({KeyEvent::Code::Enter, 0});

    LineEditor editor(term, "> ", &h);
    std::string line;
    CHECK(editor.read_line(line, no_candidates, no_hint));
    CHECK(line == "only");
}

TEST_CASE("LineEditor Down with empty history is no-op")
{
    InMemoryHistory h;

    ScriptedTerminal term;
    chars(term, "ab");
    press_down(term);
    term.keys.push_back({KeyEvent::Code::Enter, 0});

    LineEditor editor(term, "> ", &h);
    std::string line;
    CHECK(editor.read_line(line, no_candidates, no_hint));
    CHECK(line == "ab");
}

TEST_CASE("LineEditor Up then Down restores empty draft")
{
    InMemoryHistory h;
    h.push("a");
    h.push("b");

    ScriptedTerminal term;
    press_up(term);
    press_down(term);
    term.keys.push_back({KeyEvent::Code::Enter, 0});

    LineEditor editor(term, "> ", &h);
    std::string line;
    CHECK(editor.read_line(line, no_candidates, no_hint));
    CHECK(line.empty());
}

TEST_CASE("LineEditor recalls entry across read_line calls")
{
    InMemoryHistory h;
    h.push("a");
    h.push("b");

    ScriptedTerminal term;
    press_up(term);
    term.keys.push_back({KeyEvent::Code::Enter, 0});
    press_up(term);
    term.keys.push_back({KeyEvent::Code::Enter, 0});

    LineEditor editor(term, "> ", &h);
    std::string line;
    CHECK(editor.read_line(line, no_candidates, no_hint));
    CHECK(line == "b");
    CHECK(editor.read_line(line, no_candidates, no_hint));
    CHECK(line == "b");
}

TEST_CASE("LineEditor history recall redraws buffer")
{
    InMemoryHistory h;
    h.push("second");

    ScriptedTerminal term;
    chars(term, "x");
    press_up(term);
    term.keys.push_back({KeyEvent::Code::Enter, 0});

    LineEditor editor(term, "> ", &h);
    std::string line;
    CHECK(editor.read_line(line, no_candidates, no_hint));
    CHECK(line == "second");
    CHECK(term.written.find("> second") != std::string::npos);
    CHECK(term.written.find("> x") == std::string::npos);
}
