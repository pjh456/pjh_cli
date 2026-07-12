#ifndef TESTS_CONSOLE_TEST_HELPERS_HPP
#define TESTS_CONSOLE_TEST_HELPERS_HPP

#include <iostream>
#include <sstream>
#include <string>

struct CoutCapture
{
    std::stringstream buf;
    std::streambuf *old;
    CoutCapture()
        : old(std::cout.rdbuf(buf.rdbuf()))
    {
    }
    ~CoutCapture()
    {
        std::cout.rdbuf(old);
    }
    std::string str() const { return buf.str(); }
};

#endif
