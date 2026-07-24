#ifndef TESTS_CONSOLE_TEST_HELPERS_HPP
#define TESTS_CONSOLE_TEST_HELPERS_HPP

#include <iostream>
#include <sstream>
#include <string>

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

#endif