#ifndef TESTS_PARSER_TEST_HELPERS_HPP
#define TESTS_PARSER_TEST_HELPERS_HPP

#include <pjh_cli.hpp>
#include <string>
#include <vector>

using namespace pjh::cli;

struct Argv
{
    std::vector<std::string> storage;
    std::vector<char *> ptrs;

    Argv(std::initializer_list<std::string> list)
        : storage(list)
    {
        for (auto &s : storage)
            ptrs.push_back(s.data());
    }

    int argc() const { return static_cast<int>(ptrs.size()); }
    char **argv() { return ptrs.data(); }
};

#endif
