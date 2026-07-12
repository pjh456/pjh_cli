#include <pjh_cli.hpp>
#include <iostream>

using namespace pjh::cli;

int main(int argc, char **argv)
{
    App app("hello", "1.0.0", "Minimal greeting example");

    auto &name =
        app.option<
            std::string,
            fixed_string("name")>(
            "--name",
            'n',
            "Who to greet",
            std::string("world"));

    auto r = app.parse(argc, argv);
    if (r.is_err())
    {
        std::cerr
            << r.unwrap_err().what()
            << "\n";
        return 1;
    }

    auto &ctx = r.unwrap();
    std::cout << "Hello, "
              << ctx.get<
                     std::string,
                     fixed_string("name")>()
              << "!\n";
    return 0;
}
