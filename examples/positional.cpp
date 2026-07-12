#include <pjh_cli.hpp>
#include <iostream>

using namespace pjh::cli;

int main(int argc, char **argv)
{
    App app("copy", "1.0.0", "Positional arguments example");

    app.arg<std::string, 0>(
           "source",
           "Source file path")
        .required();
    app.arg<std::string, 1>(
           "dest",
           "Destination path")
        .required();
    app.option<
        bool,
        fixed_string("verbose")>(
        "--verbose",
        'v',
        "Verbose output");

    auto r = app.parse(argc, argv);
    if (r.is_err())
    {
        std::cerr
            << r.unwrap_err().what()
            << "\n";
        return 1;
    }

    auto &ctx = r.unwrap();
    auto src = ctx.get<std::string, 0>();
    auto dst = ctx.get<std::string, 1>();
    std::cout << "Copy " << src
              << " -> " << dst
              << "\n";
    return 0;
}
