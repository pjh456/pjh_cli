#include <iostream>
#include <pjh_cli/command/leaf_command.hpp>
#include <pjh_cli/core/fixed_string.hpp>
#include <pjh_cli/parse/parser.hpp>

using namespace pjh::cli;

int main(int argc, char **argv)
{
    LeafCommand app("copy", "Positional arguments example");

    app.arg<std::string, 0>("source", "Source file path").required();
    app.arg<std::string, 1>("dest", "Destination path").required();
    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose output").boolean();

    auto r = Parser::parse_command(app, argc, argv);
    if (r.is_err())
    {
        std::cerr << r.unwrap_err().what() << "\n";
        return 1;
    }

    auto &ctx = r.unwrap();
    auto src = ctx.get<std::string, 0>();
    auto dst = ctx.get<std::string, 1>();
    std::cout << "Copy " << src << " -> " << dst << "\n";
    return 0;
}
