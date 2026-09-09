#include <doctest/doctest.h>

#include <pjh_cli/console/in_memory_history.hpp>
#include <pjh_cli/console/line_editor.hpp>
#include <pjh_cli/console/noop_history.hpp>
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

    /// @brief Push one Ctrl-C (line cancel) KeyEvent.
    void press_cancel(ScriptedTerminal &term)
    {
        term.keys.push_back({KeyEvent::Code::Cancel, 0});
    }

    /// @brief Completion callback that always returns an empty candidate list.
    CompletionResult no_candidates(std::string_view, std::size_t) { return {}; }

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
        CompletionResult out;
        if (line == "ser")
            out.candidates.push_back({"serve"});
        out.prefix_len = line.size();
        return out;
    };

    LineEditor editor(term, "> ");
    std::string line;
    CHECK(editor.read_line(line, complete, no_hint));
    CHECK(line == "serve ");
    CHECK(term.written.find("ve ") != std::string::npos);
}

TEST_CASE("LineEditor Tab completes inline long option value")
{
    ScriptedTerminal term;
    chars(term, "--color=g");
    term.keys.push_back({KeyEvent::Code::Tab, 0});
    term.keys.push_back({KeyEvent::Code::Enter, 0});

    CompletionFn complete = [](std::string_view line, std::size_t)
    {
        CompletionResult out;
        if (line == "--color=g")
            out.candidates.push_back({"green"});
        out.prefix_len = 1;  // value suffix after '='.
        return out;
    };

    LineEditor editor(term, "> ");
    std::string line;
    CHECK(editor.read_line(line, complete, no_hint));
    CHECK(line == "--color=green ");
}

TEST_CASE("LineEditor Tab completes compact short option value")
{
    ScriptedTerminal term;
    chars(term, "-cgr");
    term.keys.push_back({KeyEvent::Code::Tab, 0});
    term.keys.push_back({KeyEvent::Code::Enter, 0});

    CompletionFn complete = [](std::string_view line, std::size_t)
    {
        CompletionResult out;
        if (line == "-cgr")
            out.candidates.push_back({"green"});
        out.prefix_len = 2;  // "gr" after the option char.
        return out;
    };

    LineEditor editor(term, "> ");
    std::string line;
    CHECK(editor.read_line(line, complete, no_hint));
    CHECK(line == "-cgreen ");
}

TEST_CASE("LineEditor Tab completes inline long option value from empty prefix")
{
    ScriptedTerminal term;
    chars(term, "--color=");
    term.keys.push_back({KeyEvent::Code::Tab, 0});
    term.keys.push_back({KeyEvent::Code::Enter, 0});

    CompletionFn complete = [](std::string_view line, std::size_t)
    {
        CompletionResult out;
        if (line == "--color=")
            out.candidates.push_back({"green"});
        out.prefix_len = 0;
        return out;
    };

    LineEditor editor(term, "> ");
    std::string line;
    CHECK(editor.read_line(line, complete, no_hint));
    CHECK(line == "--color=green ");
}

TEST_CASE("LineEditor Tab lists multiple candidates and hint")
{
    ScriptedTerminal term;
    chars(term, "ser");
    term.keys.push_back({KeyEvent::Code::Tab, 0});
    term.keys.push_back({KeyEvent::Code::Enter, 0});

    CompletionFn complete = [](std::string_view, std::size_t)
    {
        CompletionResult out;
        out.candidates = {{"serve"}, {"server"}};
        out.prefix_len = 3;
        return out;
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

TEST_CASE("LineEditor no-op Up on empty history preserves later edits")
{
    InMemoryHistory h;  // empty: prev() returns None.

    ScriptedTerminal term;
    chars(term, "ls");
    press_up(term);
    chars(term, " -la");
    press_down(term);
    term.keys.push_back({KeyEvent::Code::Enter, 0});

    LineEditor editor(term, "> ", &h);
    std::string line;
    CHECK(editor.read_line(line, no_candidates, no_hint));
    CHECK(line == "ls -la");
}

TEST_CASE("LineEditor no-op history Up then Down preserves later edits")
{
    NoOpHistory h;  // prev()/next() always return None.

    ScriptedTerminal term;
    chars(term, "ls");
    press_up(term);
    chars(term, " -la");
    press_down(term);
    term.keys.push_back({KeyEvent::Code::Enter, 0});

    LineEditor editor(term, "> ", &h);
    std::string line;
    CHECK(editor.read_line(line, no_candidates, no_hint));
    CHECK(line == "ls -la");
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

// ──────────────────────────────────────────
//  UTF-8 editing (task 39)
// ──────────────────────────────────────────

TEST_CASE("LineEditor backspace removes a whole CJK code point")
{
    ScriptedTerminal term;
    chars(term, "\xE4\xB8\xAD\xE6\x96\x87");  // 中文
    term.keys.push_back({KeyEvent::Code::Backspace, 0});
    term.keys.push_back({KeyEvent::Code::Enter, 0});

    LineEditor editor(term, "> ");
    std::string line;
    CHECK(editor.read_line(line, no_candidates, no_hint));
    CHECK(line == "\xE4\xB8\xAD");  // 中, valid UTF-8
    CHECK(term.written == "> \xE4\xB8\xAD\n");
}

TEST_CASE("LineEditor backspace removes mixed ASCII and CJK by code point")
{
    ScriptedTerminal term;
    chars(term, std::string("ab") + "\xE4\xB8\xAD");  // ab中
    term.keys.push_back({KeyEvent::Code::Backspace, 0});
    term.keys.push_back({KeyEvent::Code::Backspace, 0});
    term.keys.push_back({KeyEvent::Code::Enter, 0});

    LineEditor editor(term, "> ");
    std::string line;
    CHECK(editor.read_line(line, no_candidates, no_hint));
    CHECK(line == "a");
}

TEST_CASE("LineEditor backspace with lone continuation byte is graceful")
{
    ScriptedTerminal term;
    chars(term, "\x80");  // malformed byte still arrives as a Character
    term.keys.push_back({KeyEvent::Code::Backspace, 0});
    term.keys.push_back({KeyEvent::Code::Enter, 0});

    LineEditor editor(term, "> ");
    std::string line;
    CHECK(editor.read_line(line, no_candidates, no_hint));
    CHECK(line.empty());
}

TEST_CASE("LineEditor history recall of a CJK entry redraws it intact")
{
    InMemoryHistory h;
    h.push("\xE4\xB8\xAD\xE6\x96\x87");  // 中文

    ScriptedTerminal term;
    chars(term, "x");
    press_up(term);
    term.keys.push_back({KeyEvent::Code::Enter, 0});

    LineEditor editor(term, "> ", &h);
    std::string line;
    CHECK(editor.read_line(line, no_candidates, no_hint));
    CHECK(line == "\xE4\xB8\xAD\xE6\x96\x87");
    // The recalled entry must be written back as valid UTF-8 after the draft
    // "x" is erased (erase_last(1) here; the over-erase case is the next test).
    CHECK(term.written == "> \xE4\xB8\xAD\xE6\x96\x87\n");
}

TEST_CASE("LineEditor history recall over CJK draft erases code points")
{
    InMemoryHistory h;
    h.push("hello");

    ScriptedTerminal term;
    chars(term, "\xE4\xB8\xAD\xE6\x96\x87");  // 中文
    press_up(term);
    term.keys.push_back({KeyEvent::Code::Enter, 0});

    LineEditor editor(term, "> ", &h);
    std::string line;
    CHECK(editor.read_line(line, no_candidates, no_hint));
    CHECK(line == "hello");
    // On a real terminal, byte-wise erase_last(6) on "> 中文" backs the cursor
    // six cells into the prompt (only four cells exist); code-point
    // erase_last(2) leaves the prompt and rewrites.  The assertion discriminates
    // byte vs code-point counts once ScriptedTerminal is updated per §3.5.
    CHECK(term.written == "> hello\n");
}

TEST_CASE("LineEditor Tab completes a CJK candidate then backspace removes it")
{
    ScriptedTerminal term;
    chars(term, "\xE4\xB8");  // partial 中
    term.keys.push_back({KeyEvent::Code::Tab, 0});
    term.keys.push_back({KeyEvent::Code::Backspace, 0});  // trailing space
    term.keys.push_back({KeyEvent::Code::Backspace, 0});  // the completed 中
    term.keys.push_back({KeyEvent::Code::Enter, 0});

    CompletionFn complete = [](std::string_view line, std::size_t)
    {
        CompletionResult out;
        if (line == "\xE4\xB8")
            out.candidates.push_back({"\xE4\xB8\xAD"});  // 中
        out.prefix_len = line.size();
        return out;
    };

    LineEditor editor(term, "> ");
    std::string line;
    CHECK(editor.read_line(line, complete, no_hint));
    CHECK(line.empty());  // both the appended space and 中 are removed whole
}

// ──────────────────────────────────────────
//  Ctrl-C line cancel (task 50)
// ──────────────────────────────────────────

TEST_CASE("LineEditor Ctrl-C discards the current line and reads the next")
{
    ScriptedTerminal term;
    chars(term, "bad");
    press_cancel(term);
    chars(term, "serve");
    term.keys.push_back({KeyEvent::Code::Enter, 0});

    LineEditor editor(term, "> ");
    std::string line;
    CHECK(editor.read_line(line, no_candidates, no_hint));
    CHECK(line == "serve");
    CHECK(term.written.find("^C\n") != std::string::npos);
    CHECK(term.written.find("> serve\n") != std::string::npos);
}

TEST_CASE("LineEditor Ctrl-C on an empty line redraws the prompt")
{
    ScriptedTerminal term;
    press_cancel(term);
    term.keys.push_back({KeyEvent::Code::Enter, 0});

    LineEditor editor(term, "> ");
    std::string line;
    CHECK(editor.read_line(line, no_candidates, no_hint));
    CHECK(line.empty());
    CHECK(term.written == "> ^C\n> \n");
}

TEST_CASE("LineEditor Ctrl-C resets history navigation")
{
    ScriptedTerminal term;
    chars(term, "abc");
    press_up(term);      // recall newest ("two"), arms draft
    press_cancel(term);  // discard + reset cursor/nav/draft
    press_up(term);      // must recall newest again, not the older entry
    term.keys.push_back({KeyEvent::Code::Enter, 0});

    InMemoryHistory history;
    history.push("one");
    history.push("two");

    LineEditor editor(term, "> ", &history);
    std::string line;
    CHECK(editor.read_line(line, no_candidates, no_hint));
    CHECK(line == "two");
}
