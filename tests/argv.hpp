#ifndef TESTS_ARGV_HPP
#define TESTS_ARGV_HPP

#include <initializer_list>
#include <string>
#include <vector>

struct Argv
{
    std::vector<std::string> storage;
    std::vector<char *> ptrs;

    Argv(std::initializer_list<std::string> list) : storage(list)
    {
        for (auto &s : storage) ptrs.push_back(s.data());
    }

    int argc() const { return static_cast<int>(ptrs.size()); }
    char **argv() { return ptrs.data(); }
};

#endif  // TESTS_ARGV_HPP
