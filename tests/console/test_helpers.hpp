#ifndef TESTS_CONSOLE_TEST_HELPERS_HPP
#define TESTS_CONSOLE_TEST_HELPERS_HPP

#include <cstddef>
#include <deque>
#include <iostream>
#include <memory>
#include <pjh_cli/console/line_editor.hpp>
#include <pjh_cli/detail/string_utils.hpp>
#include <sstream>
#include <string>
#include <string_view>

/// @brief Temporarily redirects std::cout to a stringstream for test assertions.
///
/// Captures all std::cout output during its lifetime via rdbuf() swap.
/// Restores the original buffer on destruction.
///
/// Prefer StreamFixture for new tests; CoutCapture is retained for
/// backward compatibility with any existing test that calls free
/// functions writing to std::cout directly.
struct CoutCapture
{
    std::stringstream buf;
    std::streambuf *old;
    CoutCapture() : old(std::cout.rdbuf(buf.rdbuf())) {}
    ~CoutCapture() { std::cout.rdbuf(old); }
    std::string str() const { return buf.str(); }
};

/// @brief Pre-wired stringstream triple for InteractiveConsole testing.
///
/// Provides separate input, output, and error streams that can be
/// injected into InteractiveConsole's constructor.  After calling
/// process_line() or run(), assert against output.str() and
/// error.str() without any global rdbuf manipulation.
struct StreamFixture
{
    std::stringstream input;
    std::stringstream output;
    std::stringstream error;
};

/// @brief Scripted ITerminal for headless LineEditor / InteractiveConsole tests.
///
/// Pre-load @c keys with KeyEvents; read_key() pops them and returns Eof once
/// empty.  @c written accumulates echoed output, and erase_last() removes the
/// requested characters from the end so the log models the visible line.
class ScriptedTerminal : public pjh::cli::ITerminal
{
public:
    ~ScriptedTerminal() override { *alive = false; }

    pjh::cli::KeyEvent read_key() override
    {
        ++read_calls;
        if (keys.empty())
            return {pjh::cli::KeyEvent::Code::Eof, 0};
        auto event = keys.front();
        keys.pop_front();
        return event;
    }

    void write(std::string_view text) override { written.append(text); }

    void erase_last(std::size_t count) override
    {
        for (std::size_t i = 0; i < count && !written.empty(); ++i)
            written.resize(
                pjh::cli::detail::utf8_prev_code_point(written, written.size()));
    }

    void suspend() override { ++suspend_calls; }

    void resume() override { ++resume_calls; }

    std::deque<pjh::cli::KeyEvent> keys;  ///< Scripted key sequence.
    std::string written;                  ///< Accumulated echo output.
    std::size_t suspend_calls = 0;        ///< Times suspend() was invoked.
    std::size_t resume_calls = 0;         ///< Times resume() was invoked.
    std::shared_ptr<bool> alive =
        std::make_shared<bool>(true);  ///< False once destroyed.
    std::size_t read_calls = 0;        ///< read_key() invocations.
};

#endif